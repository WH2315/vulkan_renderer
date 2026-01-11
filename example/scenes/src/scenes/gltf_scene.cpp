#include "scenes/gltf_scene.hpp"
#include <random>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

void GLTFScene::initialize() {
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

    scene_ =
        interface->loadGLTFScene("Sponza/glTF/Sponza.gltf", {"NORMAL", "TEXCOORD_0"});
    model1_ = interface->loadNormalModel("ray_tracing/sphere.obj");
    model2_ = interface->loadNormalModel("dragon.obj");
    material_ = Material{.albedo = glm::vec3(0.8f, 0.5f, 0.25f),
                         .roughness = 0.6f,
                         .specular_albedo = glm::vec3(1.0f),
                         .specular_probability = 0.2f,
                         .emissive_color = glm::vec3(1.0f, 0.9f, 0.8f),
                         .emissive_intensity = 0.5f};
    as_ = interface->createAccelerationStructure();
    as_->addModel(model1_);
    as_->addModel(model2_);
    as_->addGLTFScene(scene_);
    as_->build(false, false);
    as_.reset();
    rt_instance_ = interface->createRayTracingInstance();
    rt_instance_->registerCustomInstanceData<Material>();
    rt_instance_->addNormalModel(0, 0, model1_, glm::mat4(1.0f));
    std::random_device device;
    std::mt19937 generator(device());
    std::normal_distribution<float> distribution(0, 0.8f);
    std::normal_distribution<float> scaleDistribution(0.3f, 0.2f);
    std::uniform_real_distribution<float> colorDistribution(0, 1);
    for (int i = 0; i < 200; i++) {
        auto position = glm::vec3(distribution(generator) * 4,
                                  distribution(generator) > 0 ? 4 : 0.5f,
                                  distribution(generator) * 2);
        auto scale = scaleDistribution(generator);
        auto transform =
            glm::translate(position) * glm::scale(glm::mat4(1.0f), glm::vec3(scale));
        auto material = Material{
            .emissive_color =
                {
                                 colorDistribution(generator),
                                 colorDistribution(generator),
                                 colorDistribution(generator),
                                 },
            .emissive_intensity = 0.8f,
        };
        rt_instance_->addNormalModel(1, 0, model1_, transform, material);
    }
    rt_instance_->addNormalModel(1, 0, model2_, glm::translate(glm::vec3(-1, 3, 0)));
    rt_instance_->addGLTFScene(2, 1, scene_);
    rt_instance_->build(true);

    // camera
    camera_ = std::make_unique<Camera>();
    camera_->setViewportSize(viewport_size.x, viewport_size.y);
    camera_->setInitialState({2.0f, 2.0f, 0.0f}, {-1.0f, 0.0f, 0.0f});

    pcs_ = interface->createPushConstants(
        wen::ShaderStage::eRaygen,
        {
            {                   "time", wen::ConstantType::eFloat},
            {            "frame_index", wen::ConstantType::eInt32},
            {           "sample_count", wen::ConstantType::eInt32},
            {             "view_depth", wen::ConstantType::eFloat},
            {    "view_depth_strength", wen::ConstantType::eFloat},
            {"max_ray_recursion_depth", wen::ConstantType::eInt32}
    });

    image_ = interface->createStorageImage(viewport_size.x, viewport_size.y,
                                           vk::Format::eR32G32B32A32Sfloat,
                                           vk::ImageUsageFlagBits::eSampled);
    sampler_ = interface->createSampler();

    auto raygen =
        interface->loadShader("gltf_scene/gltf.rgen", wen::ShaderStage::eRaygen);
    auto miss = interface->loadShader("gltf_scene/gltf.rmiss", wen::ShaderStage::eMiss);
    auto material = interface->loadShader("gltf_scene/material.rchit",
                                          wen::ShaderStage::eClosestHit);
    auto closest =
        interface->loadShader("gltf_scene/gltf.rchit", wen::ShaderStage::eClosestHit);

    rt_sp_ = interface->createRayTracingShaderProgram();
    rt_sp_->setRaygenShader(raygen);
    rt_sp_->setMissShader(miss);
    rt_sp_->setHitGroup({material, std::nullopt});
    rt_sp_->setHitGroup({closest, std::nullopt});
    rt_ds_ = interface->createDescriptorSet();
    rt_ds_->addDescriptors({
        // camera
        {0, vk::DescriptorType::eUniformBuffer, wen::ShaderStage::eRaygen},
        // rt_instance_
        {1, vk::DescriptorType::eAccelerationStructureKHR,
         wen::ShaderStage::eRaygen | wen::ShaderStage::eClosestHit},
        // output image
        {2, vk::DescriptorType::eStorageImage, wen::ShaderStage::eRaygen},
        // instance address buffer
        {3, vk::DescriptorType::eStorageBuffer, wen::ShaderStage::eClosestHit},
        // custom material data buffer
        {4, vk::DescriptorType::eStorageBuffer, wen::ShaderStage::eClosestHit},
        // GLTF: primitive data buffer
        {5, vk::DescriptorType::eStorageBuffer, wen::ShaderStage::eClosestHit},
        // material buffer
        {6, vk::DescriptorType::eStorageBuffer, wen::ShaderStage::eClosestHit},
        // NORMAL
        {7, vk::DescriptorType::eStorageBuffer, wen::ShaderStage::eClosestHit},
        // TEXCOORD_0
        {8, vk::DescriptorType::eStorageBuffer, wen::ShaderStage::eClosestHit},
        // all textures
        {9, vk::DescriptorType::eCombinedImageSampler, scene_->getTexturesCount(),
         wen::ShaderStage::eClosestHit}
    });
    rt_ds_->build();
    rt_ds_->bindUniform(0, camera_->uniform_buffer);
    rt_ds_->bindAccelerationStructure(1, rt_instance_);
    rt_ds_->bindStorageImage(2, image_);
    rt_ds_->bindStorageBuffer(3, rt_instance_->getInstanceAddressBuffer());
    rt_ds_->bindStorageBuffer(4, rt_instance_->getCustomInstanceDataBuffer<Material>());
    rt_ds_->bindStorageBuffer(5, rt_instance_->getPrimitiveDataBuffer());
    rt_ds_->bindStorageBuffer(6, scene_->getMaterialBuffer());
    rt_ds_->bindStorageBuffer(7, scene_->getAttrBuffer("NORMAL"));
    rt_ds_->bindStorageBuffer(8, scene_->getAttrBuffer("TEXCOORD_0"));
    scene_->bindTexturesSamplers(rt_ds_, 9);
    rt_rp_ = interface->createRayTracingRenderPipeline(rt_sp_);
    rt_rp_->setDescriptorSet(rt_ds_, 0);
    rt_rp_->setPushConstants(pcs_);
    rt_rp_->compile({.max_ray_recursion_depth = 1});

    auto vs =
        interface->loadShader("gltf_scene/shader.vert", wen::ShaderStage::eVertex);
    auto fs =
        interface->loadShader("gltf_scene/shader.frag", wen::ShaderStage::eFragment);

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

void GLTFScene::recreateImageOutput() {
    image_ = interface->createStorageImage(viewport_size.x, viewport_size.y,
                                           vk::Format::eR32G32B32A32Sfloat,
                                           vk::ImageUsageFlagBits::eSampled);
    rt_ds_->bindStorageImage(2, image_);
    image_ds_->bindTexture(0, image_, sampler_);
}

void GLTFScene::update(float ts, float w, float h) {
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

    rt_instance_->update(0, [&](uint32_t index, auto updateTransform, auto) {
        updateTransform(glm::translate(model_position_) *
                        glm::scale(glm::mat4(1.0f), glm::vec3(model_scale_)));
    });
    rt_instance_->update<Material>(
        0, [&](uint32_t index, auto& material) { material = material_; });
}

void GLTFScene::render(float w, float h) {
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

void GLTFScene::imgui() {
    ImGui::Begin("Settings");

    ImGui::Text("FrameRate: %f", ImGui::GetIO().Framerate);
    ImGui::Text("FrameIndex: %d", frame_index_);

    bool changed = false;

    static int sample_count = 3;
    changed |= ImGui::SliderInt("Sample Count", &sample_count, 1, 16);
    pcs_->pushConstant("sample_count", &sample_count);

    static float view_depth = 1.0f;
    changed |= ImGui::SliderFloat("View Depth", &view_depth, 0.01, 5);
    pcs_->pushConstant("view_depth", &view_depth);

    static float view_depth_strength = 0.01f;
    changed |= ImGui::SliderFloat("View Depth Strength", &view_depth_strength, 0, 0.5);
    pcs_->pushConstant("view_depth_strength", &view_depth_strength);

    static int max_ray_recursion_depth = 6;
    changed |=
        ImGui::SliderInt("Max Ray Recursion Depth", &max_ray_recursion_depth, 1, 16);
    pcs_->pushConstant("max_ray_recursion_depth", &max_ray_recursion_depth);

    ImGui::SeparatorText("Model Material");
    changed |= ImGui::DragFloat3("Model Position", &model_position_.x, 0.01f);
    changed |= ImGui::SliderFloat("Model Scale", &model_scale_, 0.1f, 10.0f);
    changed |= ImGui::ColorEdit3("Albedo", &material_.albedo.r);
    changed |= ImGui::SliderFloat("Roughness", &material_.roughness, 0, 1);
    changed |= ImGui::ColorEdit3("Specular Albedo", &material_.specular_albedo.r);
    changed |= ImGui::SliderFloat("Specular Probability",
                                  &material_.specular_probability, 0, 1);
    changed |= ImGui::ColorEdit3("Emissive Color", &material_.emissive_color.r);
    changed |=
        ImGui::SliderFloat("Emissive Intensity", &material_.emissive_intensity, 0, 1);

    if (changed) {
        frame_index_ = 0;
    }

    ImGui::End();
}

void GLTFScene::destroy() {}