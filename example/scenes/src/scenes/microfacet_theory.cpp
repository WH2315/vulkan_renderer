#include "scenes/microfacet_theory.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

void MicrofacetTheory::initialize() {
    auto render_pass = interface->createRenderPass(false);
    render_pass->addAttachment(wen::SWAPCHAIN_IMAGE_ATTACHMENT,
                               wen::AttachmentType::eColor);
    render_pass->addAttachment(wen::DEPTH_ATTACHMENT, wen::AttachmentType::eDepth);
    render_pass->addAttachment(wen::IMGUI_DOCKING_ATTACHMENT,
                               wen::AttachmentType::eRGBA8Snorm);

    auto& subpass = render_pass->addSubpass("main_subpass");
    subpass.setOutputAttachment(wen::IMGUI_DOCKING_ATTACHMENT);
    subpass.setDepthAttachment(wen::DEPTH_ATTACHMENT);

    render_pass->addSubpassDependency(
        wen::EXTERNAL_SUBPASS, "main_subpass",
        {vk::PipelineStageFlagBits::eColorAttachmentOutput |
             vk::PipelineStageFlagBits::eLateFragmentTests,
         vk::PipelineStageFlagBits::eColorAttachmentOutput |
             vk::PipelineStageFlagBits::eLateFragmentTests},
        {vk::AccessFlagBits::eColorAttachmentWrite |
             vk::AccessFlagBits::eDepthStencilAttachmentWrite,
         vk::AccessFlagBits::eColorAttachmentWrite |
             vk::AccessFlagBits::eDepthStencilAttachmentWrite});

    render_pass->build();

    renderer = interface->createRenderer(std::move(render_pass));
    imGui = std::make_shared<wen::Imgui>(*renderer, true);

    sphere_ = interface->createSphereModel();
    sphere_->registerCustomSphereData<MicrofacetTheoryMaterial>();
    // 电解质
    dielectric_material_.albedo = {216.f / 255.f, 148.f / 255.f, 235.f / 255.f};
    dielectric_material_.type = MicrofacetTheoryMaterial::Type::eDielectric;
    dielectric_material_.IOR = 2.0f;
    dielectric_material_.K = 0.0f;
    dielectric_material_.alpha_x = 0.5f;
    dielectric_material_.alpha_y = 0.05f;
    sphere_->addSphereModel(0, {-1.5, 1, 0}, 0.9, dielectric_material_);
    // 导体
    conductor_material_.albedo = {189.f / 255.f, 143.f / 255.f, 198.f / 255.f};
    conductor_material_.type = MicrofacetTheoryMaterial::Type::eConductor;
    conductor_material_.IOR = 2.0f;
    conductor_material_.K = 3.5f;
    conductor_material_.alpha_x = 0.05f;
    conductor_material_.alpha_y = 0.5f;
    sphere_->addSphereModel(0, {1.5, 1, 0}, 0.9, conductor_material_);
    // 地面
    MicrofacetTheoryMaterial ground_material{};
    ground_material.albedo = {0.8f, 0.5f, 0.25f};
    sphere_->addSphereModel(0, {0, -500, 0}, 500, ground_material);

    bunny_d1_material_ = {
        .albedo = {131. / 255., 182. / 255., 224. / 255.},
        .type = MicrofacetTheoryMaterial::Type::eDielectric,
        .IOR = 2.5,
        .K = 0,
        .alpha_x = 0.1,
        .alpha_y = 0.1,
    };
    bunny_d2_material_ = {
        .albedo = {210. / 255., 115. / 255., 211. / 255.},
        .type = MicrofacetTheoryMaterial::Type::eDielectric,
        .IOR = 1.2,
        .K = 0,
        .alpha_x = 0,
        .alpha_y = 0,
    };
    bunny_c_material_ = {
        .albedo = {231. / 255., 235. / 255., 173. / 255.},
        .type = MicrofacetTheoryMaterial::Type::eConductor,
        .IOR = 1.5,
        .K = 3,
        .alpha_x = 0,
        .alpha_y = 0.2,
    };
    bunny_ = interface->loadNormalModel("bunny.obj");

    outter_d_material_ = {
        .albedo = {222. / 255., 91. / 255., 36. / 255.},
        .type = MicrofacetTheoryMaterial::Type::eDielectric,
        .IOR = 2,
        .K = 0,
        .alpha_x = 0.02,
        .alpha_y = 0.02,
    };
    outter_c_material_ = {
        .albedo = {222. / 255., 91. / 255., 36. / 255.},
        .type = MicrofacetTheoryMaterial::Type::eConductor,
        .IOR = 2,
        .K = 3,
        .alpha_x = 0.02,
        .alpha_y = 0.02,
    };
    inner_material_ = {
        .albedo = {1, 1, 1},
        .type = MicrofacetTheoryMaterial::Type::eDielectric,
        .IOR = 2.5,
        .K = 0,
        .alpha_x = 0.4,
        .alpha_y = 0.4,
    };
    outter_ = interface->loadNormalModel("mitsuba.obj", {"1", "2"});
    inner_ = interface->loadNormalModel("mitsuba.obj", {"1", "3"});

    dragon_material_ = {
        .albedo = {108. / 255., 129. / 255., 78. / 255.},
        .type = MicrofacetTheoryMaterial::Type::eDielectric,
        .IOR = 1.5,
        .K = 0,
        .alpha_x = 0.1,
        .alpha_y = 0.1,
    };
    dragon_ = interface->loadNormalModel("dragon.obj");

    as_ = interface->createAccelerationStructure();
    as_->addModel(sphere_);
    as_->addModel(bunny_);
    as_->addModel(outter_);
    as_->addModel(inner_);
    as_->addModel(dragon_);
    as_->build(false, false);
    as_.reset();

    rt_instance_ = interface->createRayTracingInstance();
    rt_instance_->registerCustomInstanceData<MicrofacetTheoryMaterial>();
    rt_instance_->addNormalModel(0, 0, sphere_, glm::mat4(1));
    rt_instance_->addNormalModel(1, 1, bunny_, glm::translate(glm::vec3(-2, 0.1, 3)),
                                 bunny_d1_material_);
    rt_instance_->addNormalModel(1, 1, bunny_, glm::translate(glm::vec3(0, 0.1, 3)),
                                 bunny_d2_material_);
    rt_instance_->addNormalModel(1, 1, bunny_, glm::translate(glm::vec3(2, 0.1, 3)),
                                 bunny_c_material_);
    rt_instance_->addNormalModel(1, 1, outter_, glm::translate(glm::vec3(-2.7, 0.1, 6)),
                                 outter_d_material_);
    rt_instance_->addNormalModel(1, 1, inner_, glm::translate(glm::vec3(-2.7, .1, 6.1)),
                                 inner_material_);
    rt_instance_->addNormalModel(1, 1, outter_, glm::translate(glm::vec3(2.7, 0.1, 6)),
                                 outter_c_material_);
    rt_instance_->addNormalModel(1, 1, inner_, glm::translate(glm::vec3(2.7, 0.1, 6.1)),
                                 inner_material_);
    rt_instance_->addNormalModel(
        1, 1, dragon_,
        glm::translate(glm::vec3(0.1, 0.8, 6)) *
            glm::rotate(glm::radians(90.f), glm::vec3(0, 1, 0)) *
            glm::scale(glm::vec3(2.5f, 2.5f, 2.5f)),
        dragon_material_);
    rt_instance_->build(true);

    camera_ = std::make_unique<Camera>();
    camera_->setViewportSize(viewport_size.x, viewport_size.y);
    camera_->setInitialState({0.0f, 1.0f, -3.0f}, {0.0f, 0.0f, 1.0f});

    pcs_ = interface->createPushConstants(
        wen::ShaderStage::eRaygen,
        {
            {               "time", wen::ConstantType::eFloat},
            {        "frame_index", wen::ConstantType::eInt32},
            {       "sample_count", wen::ConstantType::eInt32},
            {         "view_depth", wen::ConstantType::eFloat},
            {"view_depth_strength", wen::ConstantType::eFloat},
            {               "prob", wen::ConstantType::eFloat}
    });

    image_ = interface->createStorageImage(viewport_size.x, viewport_size.y,
                                           vk::Format::eR32G32B32A32Sfloat,
                                           vk::ImageUsageFlagBits::eSampled);
    sampler_ = interface->createSampler();

    auto rt_rgen =
        interface->loadShader("microfacet_theory/rt.rgen", wen::ShaderStage::eRaygen);
    auto rt_rmiss =
        interface->loadShader("microfacet_theory/rt.rmiss", wen::ShaderStage::eMiss);
    auto sphere_rchit = interface->loadShader("microfacet_theory/sphere.rchit",
                                              wen::ShaderStage::eClosestHit);
    auto sphere_rint = interface->loadShader("microfacet_theory/sphere.rint",
                                             wen::ShaderStage::eIntersection);
    auto triangle_rchit = interface->loadShader("microfacet_theory/triangle.rchit",
                                                wen::ShaderStage::eClosestHit);

    rt_sp_ = interface->createRayTracingShaderProgram();
    rt_sp_->setRaygenShader(rt_rgen);
    rt_sp_->setMissShader(rt_rmiss);
    rt_sp_->setHitGroup({sphere_rchit, sphere_rint});
    rt_sp_->setHitGroup({triangle_rchit, std::nullopt});
    rt_ds_ = interface->createDescriptorSet();
    rt_ds_->addDescriptors({
        // camera
        {0,            vk::DescriptorType::eUniformBuffer,wen::ShaderStage::eRaygen                                                          },
        // rt_instance_
        {1, vk::DescriptorType::eAccelerationStructureKHR,
         wen::ShaderStage::eRaygen | wen::ShaderStage::eClosestHit                      },
        // output image
        {2,             vk::DescriptorType::eStorageImage,     wen::ShaderStage::eRaygen},
        // sphere data buffer
        {3,            vk::DescriptorType::eStorageBuffer,
         wen::ShaderStage::eClosestHit | wen::ShaderStage::eIntersection                },
        // custom sphere MicrofacetTheoryMaterial data buffer
        {4,            vk::DescriptorType::eStorageBuffer, wen::ShaderStage::eClosestHit},
        // instance address buffer
        {5,            vk::DescriptorType::eStorageBuffer, wen::ShaderStage::eClosestHit},
        // custom MicrofacetTheoryMaterial data buffer
        {6,            vk::DescriptorType::eStorageBuffer, wen::ShaderStage::eClosestHit}
    });
    rt_ds_->build();
    rt_ds_->bindUniform(0, camera_->uniform_buffer);
    rt_ds_->bindAccelerationStructure(1, rt_instance_);
    rt_ds_->bindStorageImage(2, image_);
    rt_ds_->bindStorageBuffer(3, sphere_->getSphereDataBuffer());
    rt_ds_->bindStorageBuffer(
        4, sphere_->getCustomSphereDataBuffer<MicrofacetTheoryMaterial>());
    rt_ds_->bindStorageBuffer(5, rt_instance_->getInstanceAddressBuffer());
    rt_ds_->bindStorageBuffer(
        6, rt_instance_->getCustomInstanceDataBuffer<MicrofacetTheoryMaterial>());
    rt_rp_ = interface->createRayTracingRenderPipeline(rt_sp_);
    rt_rp_->setDescriptorSet(rt_ds_, 0);
    rt_rp_->setPushConstants(pcs_);
    // 因为提取了光线递归，所以最大深度为 1，这样可以加速光追渲染
    rt_rp_->compile({.max_ray_recursion_depth = 1});

    auto vs = interface->loadShader("microfacet_theory/shader.vert",
                                    wen::ShaderStage::eVertex);
    auto fs = interface->loadShader("microfacet_theory/shader.frag",
                                    wen::ShaderStage::eFragment);

    graphics_sp_ = interface->createGraphicsShaderProgram();
    graphics_sp_->attach(vs).attach(fs);
    image_ds_ = interface->createDescriptorSet();
    image_ds_->addDescriptors({
        {0, vk::DescriptorType::eCombinedImageSampler, wen::ShaderStage::eFragment}
    });
    image_ds_->build();
    image_ds_->bindTexture(0, image_, sampler_);
    graphics_rp_ =
        interface->createGraphicsRenderPipeline(renderer, graphics_sp_, "main_subpass");
    graphics_rp_->setDescriptorSet(image_ds_, 0);
    graphics_rp_->compile({
        .depth_test_enable = true,
        .dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor}
    });

    last_w = viewport_size.x, last_h = viewport_size.y;
}

void MicrofacetTheory::recreateImageOutput() {
    image_ = interface->createStorageImage(viewport_size.x, viewport_size.y,
                                           vk::Format::eR32G32B32A32Sfloat,
                                           vk::ImageUsageFlagBits::eSampled);
    rt_ds_->bindStorageImage(2, image_);
    image_ds_->bindTexture(0, image_, sampler_);
}

void MicrofacetTheory::update(float ts, float w, float h) {
    camera_->setViewportSize(w, h);
    camera_->upload();
    camera_->update(ts);

    if (last_w != w || last_h != h) {
        recreateImageOutput();
        last_w = w;
        last_h = h;
    }

    static float time = 0;
    time += ts;
    pcs_->pushConstant("time", &time);

    if (camera_->is_cursor_locked) {
        frame_index_ = 0;
    }
    pcs_->pushConstant("frame_index", &frame_index_);
    frame_index_++;

    sphere_->update<MicrofacetTheoryMaterial>(0,
                                              [this](uint32_t index, auto& material) {
                                                  if (index == 0) {
                                                      material = dielectric_material_;
                                                  } else if (index == 1) {
                                                      material = conductor_material_;
                                                  }
                                              });
    rt_instance_->update<MicrofacetTheoryMaterial>(
        1, [this](uint32_t index, auto& material) {
            if (index == 0) {
                material = bunny_d1_material_;
            } else if (index == 1) {
                material = bunny_d2_material_;
            } else if (index == 2) {
                material = bunny_c_material_;
            } else if (index == 3) {
                material = outter_d_material_;
            } else if (index == 4) {
                material = inner_material_;
            } else if (index == 5) {
                material = outter_c_material_;
            } else if (index == 6) {
                material = inner_material_;
            } else if (index == 7) {
                material = dragon_material_;
            }
        });
}

void MicrofacetTheory::render(float w, float h) {
    renderer->bindPipeline(rt_rp_);
    renderer->bindDescriptorSets(rt_rp_);
    renderer->pushConstants(rt_rp_);
    renderer->traceRays(rt_rp_, w, h, 1);
    renderer->beginRenderPass();
    renderer->bindPipeline(graphics_rp_);
    renderer->bindDescriptorSets(graphics_rp_);
    renderer->setViewport(0, h, w, -h);
    renderer->setScissor(0, 0, static_cast<uint32_t>(w), static_cast<uint32_t>(h));
    renderer->draw(3, 1, 0, 0);
}

void MicrofacetTheory::imgui() {
    ImGui::Begin("Settings");

    ImGui::Text("FrameRate: %f", ImGui::GetIO().Framerate);
    ImGui::Text("FrameIndex: %d", frame_index_);

    bool changed = false;

    static int sample_count = 4;
    changed |= ImGui::SliderInt("Sample Count", &sample_count, 1, 16);
    pcs_->pushConstant("sample_count", &sample_count);

    static float view_depth = 1.0f;
    changed |= ImGui::SliderFloat("View Depth", &view_depth, 0.01, 5);
    pcs_->pushConstant("view_depth", &view_depth);

    static float view_depth_strength = 0.00f;
    changed |= ImGui::SliderFloat("View Depth Strength", &view_depth_strength, 0, 0.5);
    pcs_->pushConstant("view_depth_strength", &view_depth_strength);

    static float prob = 0.8f;
    changed |= ImGui::SliderFloat("Prob", &prob, 0, 1);
    pcs_->pushConstant("prob", &prob);

    ImGui::SeparatorText("Dielectric Sphere");
    changed |= ImGui::ColorEdit3("DS Albedo", &dielectric_material_.albedo.r);
    changed |= ImGui::SliderFloat("DS IOR", &dielectric_material_.IOR, 1, 3);
    changed |= ImGui::SliderFloat("DS Alpha X", &dielectric_material_.alpha_x, 0, 1);
    changed |= ImGui::SliderFloat("DS Alpha Y", &dielectric_material_.alpha_y, 0, 1);

    ImGui::SeparatorText("Conductor Sphere");
    changed |= ImGui::ColorEdit3("CS Albedo", &conductor_material_.albedo.r);
    changed |= ImGui::SliderFloat("CS IOR", &conductor_material_.IOR, 1, 3);
    changed |= ImGui::SliderFloat("CS K", &conductor_material_.K, 0, 5);
    changed |= ImGui::SliderFloat("CS Alpha X", &conductor_material_.alpha_x, 0, 1);
    changed |= ImGui::SliderFloat("CS Alpha Y", &conductor_material_.alpha_y, 0, 1);

    ImGui::SeparatorText("Dielectric Bunny1");
    changed |= ImGui::ColorEdit3("DB1 Albedo", &bunny_d1_material_.albedo.r);
    changed |= ImGui::SliderFloat("DB1 IOR", &bunny_d1_material_.IOR, 1, 3);
    changed |= ImGui::SliderFloat("DB1 Alpha X", &bunny_d1_material_.alpha_x, 0, 1);
    changed |= ImGui::SliderFloat("DB1 Alpha Y", &bunny_d1_material_.alpha_y, 0, 1);

    ImGui::SeparatorText("Dielectric Bunny2");
    changed |= ImGui::ColorEdit3("DB2 Albedo", &bunny_d2_material_.albedo.r);
    changed |= ImGui::SliderFloat("DB2 IOR", &bunny_d2_material_.IOR, 1, 3);
    changed |= ImGui::SliderFloat("DB2 Alpha X", &bunny_d2_material_.alpha_x, 0, 1);
    changed |= ImGui::SliderFloat("DB2 Alpha Y", &bunny_d2_material_.alpha_y, 0, 1);

    ImGui::SeparatorText("Conductor Bunny");
    changed |= ImGui::ColorEdit3("CB Albedo", &bunny_c_material_.albedo.r);
    changed |= ImGui::SliderFloat("CB IOR", &bunny_c_material_.IOR, 1, 3);
    changed |= ImGui::SliderFloat("CB K", &bunny_c_material_.K, 0, 5);
    changed |= ImGui::SliderFloat("CB Alpha X", &bunny_c_material_.alpha_x, 0, 1);
    changed |= ImGui::SliderFloat("CB Alpha Y", &bunny_c_material_.alpha_y, 0, 1);

    ImGui::SeparatorText("Dielectric Outter");
    changed |= ImGui::ColorEdit3("DO Albedo", &outter_d_material_.albedo.r);
    changed |= ImGui::SliderFloat("DO IOR", &outter_d_material_.IOR, 1, 3);
    changed |= ImGui::SliderFloat("DO Alpha X", &outter_d_material_.alpha_x, 0, 1);
    changed |= ImGui::SliderFloat("DO Alpha Y", &outter_d_material_.alpha_y, 0, 1);

    ImGui::SeparatorText("Conductor Outter");
    changed |= ImGui::ColorEdit3("CO Albedo", &outter_c_material_.albedo.r);
    changed |= ImGui::SliderFloat("CO IOR", &outter_c_material_.IOR, 1, 3);
    changed |= ImGui::SliderFloat("CO K", &outter_c_material_.K, 0, 5);
    changed |= ImGui::SliderFloat("CO Alpha X", &outter_c_material_.alpha_x, 0, 1);
    changed |= ImGui::SliderFloat("CO Alpha Y", &outter_c_material_.alpha_y, 0, 1);

    ImGui::SeparatorText("Inner");
    changed |= ImGui::ColorEdit3("I Albedo", &inner_material_.albedo.r);
    changed |= ImGui::SliderFloat("I IOR", &inner_material_.IOR, 1, 3);
    changed |= ImGui::SliderFloat("I Alpha X", &inner_material_.alpha_x, 0, 1);
    changed |= ImGui::SliderFloat("I Alpha Y", &inner_material_.alpha_y, 0, 1);

    ImGui::SeparatorText("Dragon");
    changed |= ImGui::ColorEdit3("D Albedo", &dragon_material_.albedo.r);
    changed |= ImGui::SliderFloat("D IOR", &dragon_material_.IOR, 1, 3);
    changed |= ImGui::SliderFloat("D Alpha X", &dragon_material_.alpha_x, 0, 1);
    changed |= ImGui::SliderFloat("D Alpha Y", &dragon_material_.alpha_y, 0, 1);

    if (changed) {
        frame_index_ = 0;
    }

    ImGui::End();
}

void MicrofacetTheory::destroy() {}