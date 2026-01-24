#include "scenes/ssao_scene.hpp"
#include <random>

void SSAO::initialize() {
    auto render_pass = interface->createRenderPass(false);
    render_pass->addAttachment(wen::SWAPCHAIN_IMAGE_ATTACHMENT,
                               wen::AttachmentType::eColor);
    render_pass->addAttachment(wen::DEPTH_ATTACHMENT, wen::AttachmentType::eDepth);
    render_pass->addAttachment("position_buffer", wen::AttachmentType::eRGBA32Sfloat);
    render_pass->addAttachment("normal_buffer", wen::AttachmentType::eRGBA32Sfloat);
    render_pass->addAttachment("ssao_buffer", wen::AttachmentType::eRGBA32Sfloat);
    render_pass->addAttachment(wen::IMGUI_DOCKING_ATTACHMENT,
                               wen::AttachmentType::eRGBA8Snorm);

    auto& prepare_subpass = render_pass->addSubpass("prepare_subpass");
    prepare_subpass.setDepthAttachment(wen::DEPTH_ATTACHMENT);
    prepare_subpass.setOutputAttachment("position_buffer");
    prepare_subpass.setOutputAttachment("normal_buffer");

    auto& ssao_subpass = render_pass->addSubpass("ssao_subpass");
    ssao_subpass.setInputAttachment("position_buffer");
    ssao_subpass.setInputAttachment("normal_buffer");
    ssao_subpass.setOutputAttachment("ssao_buffer");
    render_pass->addSubpassDependency(
        "prepare_subpass", "ssao_subpass",
        {vk::PipelineStageFlagBits::eColorAttachmentOutput,
         vk::PipelineStageFlagBits::eColorAttachmentOutput},
        {vk::AccessFlagBits::eColorAttachmentWrite,
         vk::AccessFlagBits::eColorAttachmentRead});

    auto& main_subpass = render_pass->addSubpass("main_subpass");
    main_subpass.setInputAttachment("position_buffer");
    main_subpass.setInputAttachment("normal_buffer");
    main_subpass.setInputAttachment("ssao_buffer");
    main_subpass.setOutputAttachment(wen::IMGUI_DOCKING_ATTACHMENT);
    render_pass->addSubpassDependency(
        "prepare_subpass", "main_subpass",
        {vk::PipelineStageFlagBits::eColorAttachmentOutput,
         vk::PipelineStageFlagBits::eColorAttachmentOutput},
        {vk::AccessFlagBits::eColorAttachmentWrite,
         vk::AccessFlagBits::eColorAttachmentRead});

    render_pass->build();

    renderer = interface->createRenderer(std::move(render_pass));
    imGui = std::make_shared<wen::Imgui>(*renderer, true);
    renderer->setClearColor("normal_buffer", {
                                                 {0.0f, 0.0f, 0.0f, 0.0f}
    });

    auto prepare_vs =
        interface->loadShader("ssao_scene/prepare.vert", wen::ShaderStage::eVertex);
    auto prepare_fs =
        interface->loadShader("ssao_scene/prepare.frag", wen::ShaderStage::eFragment);
    prepare_sp_ = interface->createGraphicsShaderProgram();
    prepare_sp_->attach(prepare_vs).attach(prepare_fs);
    auto deferred_vs =
        interface->loadShader("ssao_scene/deferred.vert", wen::ShaderStage::eVertex);
    auto ssao_fs =
        interface->loadShader("ssao_scene/ssao.frag", wen::ShaderStage::eFragment);
    ssao_sp_ = interface->createGraphicsShaderProgram();
    ssao_sp_->attach(deferred_vs).attach(ssao_fs);
    auto main_fs =
        interface->loadShader("ssao_scene/main.frag", wen::ShaderStage::eFragment);
    main_sp_ = interface->createGraphicsShaderProgram();
    main_sp_->attach(deferred_vs).attach(main_fs);

    auto vertex_input = interface->createVertexInput({
        {.binding = 0,
         .input_rate = wen::InputRate::eVertex,
         .formats = {
             wen::VertexType::eFloat3, // position
             wen::VertexType::eFloat3, // normal
             wen::VertexType::eFloat3  // color
         }}
    });

    model_ = interface->loadNormalModel("mori_knob.obj");
    vertex_buffer_ =
        interface->createVertexBuffer(sizeof(wen::Vertex), model_->vertex_count);
    index_buffer_ =
        interface->createIndexBuffer(wen::IndexType::eUint32, model_->index_count);
    model_->upload(vertex_buffer_, index_buffer_);

    camera_ = std::make_unique<Camera>();
    camera_->setViewportSize(viewport_size.x, viewport_size.y);
    camera_->setInitialState({0.0f, 0.0f, -3.0f}, {0.0f, 0.0f, 1.0f});

    std::uniform_real_distribution<float> float_distribution(0.0f, 1.0f);
    std::default_random_engine generator;
    std::vector<glm::vec4> ssao_samples(64);
    const int a = 4;
    std::vector<glm::vec4> ssao_random_vectors(a * a);

    ssao_pcs_ = interface->createPushConstants(
        wen::ShaderStage::eFragment, {
                                         { "window_size", wen::ConstantType::eFloat2},
                                         {           "a",  wen::ConstantType::eInt32},
                                         {"sample_count",  wen::ConstantType::eInt32},
                                         {           "r",  wen::ConstantType::eFloat}
    });
    ssao_pcs_->pushConstant("a", &a);

    main_pcs_ = interface->createPushConstants(
        wen::ShaderStage::eFragment,
        {
            {     "window_size", wen::ConstantType::eFloat2},
            {     "ssao_enable",  wen::ConstantType::eInt32},
            {"blur_kernel_size",  wen::ConstantType::eInt32}
    });

    sampler_ = interface->createSampler({
        .address_mode_u = vk::SamplerAddressMode::eRepeat,
        .address_mode_v = vk::SamplerAddressMode::eRepeat,
    });
    for (auto& sample : ssao_samples) {
        sample = glm::normalize(glm::vec4(
            // [0, 1] --> [-1, 1]
            float_distribution(generator) * 2 - 1,
            // [0, 1] --> [-1, 1]
            float_distribution(generator) * 2 - 1,
            // [0, 1] --> [0.3, 1]
            float_distribution(generator) * 0.7 + 0.3, 0));
        // 让sample更多的分布在原点附近
        float scale = float_distribution(generator) * 0.5 + 0.5;
        scale = 0.1 + 0.9 * scale * scale;
        sample *= scale;
    }
    for (auto& random_vec : ssao_random_vectors) {
        random_vec =
            glm::normalize(glm::vec4(float_distribution(generator) * 2 - 1,
                                     float_distribution(generator) * 2 - 1, 0, 0));
    }
    ssao_samples_uniform_buffer_ =
        interface->createUniformBuffer(sizeof(glm::vec4) * ssao_samples.size());
    memcpy(ssao_samples_uniform_buffer_->getData(), ssao_samples.data(),
           sizeof(glm::vec4) * ssao_samples.size());
    ssao_random_vectors_texture_ = interface->createTexture(
        reinterpret_cast<uint8_t*>(ssao_random_vectors.data()), a, a);

    // descriptor sets
    prepare_ds_ = interface->createDescriptorSet();
    prepare_ds_->addDescriptors({
        // camera
        {0, vk::DescriptorType::eUniformBuffer, wen::ShaderStage::eVertex}
    });
    prepare_ds_->build();
    prepare_ds_->bindUniform(0, camera_->uniform_buffer);
    ssao_ds_ = interface->createDescriptorSet();
    ssao_ds_->addDescriptors({
        // position
        {0, vk::DescriptorType::eCombinedImageSampler, wen::ShaderStage::eFragment},
        // normal
        {1,      vk::DescriptorType::eInputAttachment, wen::ShaderStage::eFragment},
        // ssao samples uniform buffer
        {2,        vk::DescriptorType::eUniformBuffer, wen::ShaderStage::eFragment},
        // ssao random vectors texture
        {3, vk::DescriptorType::eCombinedImageSampler, wen::ShaderStage::eFragment},
        // camera
        {4,        vk::DescriptorType::eUniformBuffer, wen::ShaderStage::eFragment}
    });
    ssao_ds_->build();
    ssao_ds_->bindInputAttachment(0, renderer, "position_buffer", sampler_);
    ssao_ds_->bindInputAttachment(1, renderer, "normal_buffer", sampler_);
    ssao_ds_->bindUniform(2, ssao_samples_uniform_buffer_);
    ssao_ds_->bindTexture(3, ssao_random_vectors_texture_, sampler_);
    ssao_ds_->bindUniform(4, camera_->uniform_buffer);
    main_ds_ = interface->createDescriptorSet();
    main_ds_->addDescriptors({
        // position
        {0,      vk::DescriptorType::eInputAttachment, wen::ShaderStage::eFragment},
        // normal
        {1,      vk::DescriptorType::eInputAttachment, wen::ShaderStage::eFragment},
        // ssao
        {2, vk::DescriptorType::eCombinedImageSampler, wen::ShaderStage::eFragment}
    });
    main_ds_->build();
    main_ds_->bindInputAttachment(0, renderer, "position_buffer", sampler_);
    main_ds_->bindInputAttachment(1, renderer, "normal_buffer", sampler_);
    main_ds_->bindInputAttachment(2, renderer, "ssao_buffer", sampler_);

    // render pipelines
    prepare_rp_ = interface->createGraphicsRenderPipeline(renderer, prepare_sp_,
                                                          "prepare_subpass");
    prepare_rp_->setVertexInput(vertex_input);
    prepare_rp_->setDescriptorSet(prepare_ds_);
    prepare_rp_->compile({
        .cull_mode = vk::CullModeFlagBits::eBack,
        .front_face = vk::FrontFace::eCounterClockwise,
        .depth_test_enable = true,
        .dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor}
    });
    ssao_rp_ =
        interface->createGraphicsRenderPipeline(renderer, ssao_sp_, "ssao_subpass");
    ssao_rp_->setDescriptorSet(ssao_ds_);
    ssao_rp_->setPushConstants(ssao_pcs_);
    ssao_rp_->compile({
        .cull_mode = vk::CullModeFlagBits::eNone,
        .depth_test_enable = false,
    });
    main_rp_ =
        interface->createGraphicsRenderPipeline(renderer, main_sp_, "main_subpass");
    main_rp_->setDescriptorSet(main_ds_);
    main_rp_->setPushConstants(main_pcs_);
    main_rp_->compile({
        .cull_mode = vk::CullModeFlagBits::eNone,
        .depth_test_enable = false,
    });
}

void SSAO::refreshInputAttachments() {
    if (!renderer || !sampler_ || !ssao_ds_ || !main_ds_) {
        return;
    }

    auto position_index =
        renderer->render_pass->getAttachmentIndex("position_buffer", true);
    auto normal_index =
        renderer->render_pass->getAttachmentIndex("normal_buffer", true);
    auto ssao_index = renderer->render_pass->getAttachmentIndex("ssao_buffer", true);

    std::array<vk::ImageView, 3> current_views{
        renderer->framebuffer_set->attachments[position_index]->image_view,
        renderer->framebuffer_set->attachments[normal_index]->image_view,
        renderer->framebuffer_set->attachments[ssao_index]->image_view,
    };

    if (current_views == last_input_views_) {
        return;
    }

    ssao_ds_->bindInputAttachment(0, renderer, "position_buffer", sampler_);
    ssao_ds_->bindInputAttachment(1, renderer, "normal_buffer", sampler_);

    main_ds_->bindInputAttachment(0, renderer, "position_buffer", sampler_);
    main_ds_->bindInputAttachment(1, renderer, "normal_buffer", sampler_);
    main_ds_->bindInputAttachment(2, renderer, "ssao_buffer", sampler_);

    last_input_views_ = current_views;
}

void SSAO::update(float ts, float w, float h) {
    camera_->setViewportSize(w, h);
    camera_->upload();
    camera_->update(ts);

    // glm::vec2 window_size = glm::vec2(w, h);
    glm::vec2 window_size = glm::vec2(
        static_cast<float>(wen::renderer_config->getWidth()),
        static_cast<float>(wen::renderer_config->getHeight()));
    ssao_pcs_->pushConstant("window_size", &window_size);
    main_pcs_->pushConstant("window_size", &window_size);
}

void SSAO::render(float w, float h) {
    refreshInputAttachments();

    w = static_cast<float>(wen::renderer_config->getWidth()),
    h = static_cast<float>(wen::renderer_config->getHeight());
    renderer->bindPipeline(prepare_rp_);
    renderer->bindDescriptorSets(prepare_rp_);
    renderer->setViewport(0, h, w, -h);
    renderer->setScissor(0, 0, static_cast<uint32_t>(w), static_cast<uint32_t>(h));
    renderer->bindVertexBuffer(vertex_buffer_);
    renderer->bindIndexBuffer(index_buffer_);
    renderer->drawModel(model_, 1, 0);
    renderer->nextSubpass();
    renderer->bindPipeline(ssao_rp_);
    renderer->bindDescriptorSets(ssao_rp_);
    renderer->pushConstants(ssao_rp_);
    renderer->drawIndexed(3, 2, 0, 0, 0);
    renderer->nextSubpass();
    renderer->bindPipeline(main_rp_);
    renderer->bindDescriptorSets(main_rp_);
    renderer->pushConstants(main_rp_);
    renderer->drawIndexed(3, 2, 0, 0, 0);
}

void SSAO::imgui() {
    ImGui::Begin("Settings");

    ImGui::Text("Framerate: %f", ImGui::GetIO().Framerate);

    static int sample_count = 32;
    ImGui::SliderInt("Sample Count", &sample_count, 1, 64);
    ssao_pcs_->pushConstant("sample_count", &sample_count);

    static float r = 0.4f;
    ImGui::SliderFloat("R", &r, 0.01, 10);
    ssao_pcs_->pushConstant("r", &r);

    static int blur_kernel_size = 4;
    ImGui::SliderInt("Blur Kernel Size", &blur_kernel_size, 1, 16);
    main_pcs_->pushConstant("blur_kernel_size", &blur_kernel_size);

    static bool ssao_enable = 0;
    ImGui::Checkbox("Enable SSAO", &ssao_enable);
    main_pcs_->pushConstant("ssao_enable", &ssao_enable);

    ImGui::End();
}

void SSAO::destroy() {}
