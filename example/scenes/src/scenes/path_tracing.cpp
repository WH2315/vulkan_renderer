#include "scenes/path_tracing.hpp"

void PathTracing::initialize() {
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

    scene_ = interface->loadGLTFScene("cornell_box/scene.gltf", {"NORMAL"});

    auto as = interface->createAccelerationStructure();
    as->addGLTFScene(scene_);
    as->build(false, false);
    as.reset();
    rt_instance_ = interface->createRayTracingInstance();
    rt_instance_->addGLTFScene(0, 0, scene_);
    rt_instance_->build(false);

    // camera
    camera_ = std::make_unique<Camera>();
    camera_->setViewportSize(viewport_size.x, viewport_size.y);
    camera_->setInitialState({0.0f, 1.0f, 3.0f}, {0.0f, 0.0f, -1.0f});

    pcs_ = interface->createPushConstants(
        wen::ShaderStage::eRaygen | wen::ShaderStage::eClosestHit,
        {
            {        "time",  wen::ConstantType::eFloat},
            {"sample_count",  wen::ConstantType::eInt32},
            { "frame_index",  wen::ConstantType::eInt32},
            {   "roughness",  wen::ConstantType::eFloat},
            {    "metallic",  wen::ConstantType::eFloat},
            {          "F0", wen::ConstantType::eFloat3},
            {   "intensity",  wen::ConstantType::eFloat},
            {        "prob",  wen::ConstantType::eFloat},
    });

    image_ = interface->createStorageImage(viewport_size.x, viewport_size.y,
                                           vk::Format::eR32G32B32A32Sfloat,
                                           vk::ImageUsageFlagBits::eSampled);
    sampler_ = interface->createSampler();

    auto rgen =
        interface->loadShader("path_tracing/rt.rgen", wen::ShaderStage::eRaygen);
    auto miss = interface->loadShader("path_tracing/rt.rmiss", wen::ShaderStage::eMiss);
    auto closest =
        interface->loadShader("path_tracing/rt.rchit", wen::ShaderStage::eClosestHit);

    rt_sp_ = interface->createRayTracingShaderProgram();
    rt_sp_->setRaygenShader(rgen);
    rt_sp_->setMissShader(miss);
    rt_sp_->setHitGroup({closest, std::nullopt});
    rt_ds_ = interface->createDescriptorSet();
    rt_ds_->addDescriptors({
        // camera
        {0,            vk::DescriptorType::eUniformBuffer,wen::ShaderStage::eRaygen                                                          },
        // rt_instance_
        {1, vk::DescriptorType::eAccelerationStructureKHR,
         wen::ShaderStage::eRaygen | wen::ShaderStage::eClosestHit                      },
        // output image
        {2,             vk::DescriptorType::eStorageImage,     wen::ShaderStage::eRaygen},
        // instance address buffer
        {3,            vk::DescriptorType::eStorageBuffer, wen::ShaderStage::eClosestHit},
        // GLTF: primitive data buffer
        {4,            vk::DescriptorType::eStorageBuffer, wen::ShaderStage::eClosestHit},
        // material buffer
        {5,            vk::DescriptorType::eStorageBuffer, wen::ShaderStage::eClosestHit},
        // NORMAL
        {6,            vk::DescriptorType::eStorageBuffer, wen::ShaderStage::eClosestHit},
    });
    rt_ds_->build();
    rt_ds_->bindUniform(0, camera_->uniform_buffer);
    rt_ds_->bindAccelerationStructure(1, rt_instance_);
    rt_ds_->bindStorageImage(2, image_);
    rt_ds_->bindStorageBuffer(3, rt_instance_->getInstanceAddressBuffer());
    rt_ds_->bindStorageBuffer(4, rt_instance_->getPrimitiveDataBuffer());
    rt_ds_->bindStorageBuffer(5, scene_->getMaterialBuffer());
    rt_ds_->bindStorageBuffer(6, scene_->getAttrBuffer("NORMAL"));
    rt_rp_ = interface->createRayTracingRenderPipeline(rt_sp_);
    rt_rp_->setDescriptorSet(rt_ds_);
    rt_rp_->setPushConstants(pcs_);
    rt_rp_->compile({.max_ray_recursion_depth = 1});

    auto vs =
        interface->loadShader("path_tracing/shader.vert", wen::ShaderStage::eVertex);
    auto fs =
        interface->loadShader("path_tracing/shader.frag", wen::ShaderStage::eFragment);
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
    graphics_rp_->setDescriptorSet(image_ds_);
    graphics_rp_->compile({
        .depth_test_enable = true,
        .dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor}
    });

    last_w = viewport_size.x, last_h = viewport_size.y;
}

void PathTracing::recreateImageOutput() {
    image_ = interface->createStorageImage(viewport_size.x, viewport_size.y,
                                           vk::Format::eR32G32B32A32Sfloat,
                                           vk::ImageUsageFlagBits::eSampled);
    rt_ds_->bindStorageImage(2, image_);
    image_ds_->bindTexture(0, image_, sampler_);
}

void PathTracing::update(float ts, float w, float h) {
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
}

void PathTracing::render(float w, float h) {
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

void PathTracing::imgui() {
    ImGui::Begin("Settings");

    ImGui::Text("FrameRate: %f", ImGui::GetIO().Framerate);
    ImGui::Text("FrameIndex: %d", frame_index_);

    bool changed = false;

    static int sample_count = 3;
    changed |= ImGui::SliderInt("Sample Count", &sample_count, 1, 16);
    pcs_->pushConstant("sample_count", &sample_count);

    static float roughness = 1;
    changed |= ImGui::SliderFloat("Roughness", &roughness, 0, 1);
    pcs_->pushConstant("roughness", &roughness);

    static float metallic = 0;
    changed |= ImGui::SliderFloat("Metallic", &metallic, 0, 1);
    pcs_->pushConstant("metallic", &metallic);

    static auto F0 = glm::vec3(0.04f);
    changed |= ImGui::ColorEdit3("F0", &F0.r);
    pcs_->pushConstant("F0", &F0.r);

    static float intensity = 3;
    changed |= ImGui::SliderFloat("Light Intensity", &intensity, 1, 100);
    pcs_->pushConstant("intensity", &intensity);

    static float prob = 0.8;
    changed |= ImGui::SliderFloat("Prob", &prob, 0.1, 0.9);
    pcs_->pushConstant("prob", &prob);

    if (changed) {
        frame_index_ = 0;
    }

    ImGui::End();
}

void PathTracing::destroy() {}