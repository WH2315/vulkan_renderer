#include "scenes/deferred_shading.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>

DeferredShading::Light::Light(std::shared_ptr<wen::Interface> interface) {
    uniform_buffer = interface->createUniformBuffer(sizeof(LightUniform));
    data = static_cast<LightUniform*>(uniform_buffer->getData());
    memset(data, 0, sizeof(LightUniform));
}

void DeferredShading::initialize() {
    auto render_pass = interface->createRenderPass(false);
    render_pass->addAttachment(wen::SWAPCHAIN_IMAGE_ATTACHMENT,
                               wen::AttachmentType::eColor);
    render_pass->addAttachment(wen::DEPTH_ATTACHMENT, wen::AttachmentType::eDepth);
    render_pass->addAttachment("position_buffer", wen::AttachmentType::eRGBA32Sfloat);
    render_pass->addAttachment("normal_buffer", wen::AttachmentType::eRGBA32Sfloat);
    render_pass->addAttachment("color_buffer", wen::AttachmentType::eRGBA32Sfloat);
    render_pass->addAttachment(wen::IMGUI_DOCKING_ATTACHMENT,
                               wen::AttachmentType::eRGBA8Snorm);

    {
        auto attachment_indices = render_pass->getAttachmentIndices();
        for (const auto& [name, index] : attachment_indices) {
            WEN_DEBUG("index: {}, attachment: {}", index, name)
        }
        auto attachments = render_pass->attachments;
        size_t i = 0;
        for (i = 0; i < attachments.size(); i++) {
            WEN_DEBUG("attachment index: {}: attachment: {}", i, attachments[i].name);
        }
        auto resolve_attachments = render_pass->resolve_attachments;
        for (; i < attachments.size() + resolve_attachments.size(); i++) {
            WEN_DEBUG("resolve_attachment index: {}, offset: {} attachment: {}", i,
                      resolve_attachments[i - attachments.size()].offset,
                      resolve_attachments[i - attachments.size()].name);
        }
    }

    auto& main_subpass = render_pass->addSubpass("main_subpass");
    main_subpass.setDepthAttachment(wen::DEPTH_ATTACHMENT);
    main_subpass.setOutputAttachment("position_buffer");
    main_subpass.setOutputAttachment("normal_buffer");
    main_subpass.setOutputAttachment("color_buffer");

    auto& post_subpass = render_pass->addSubpass("post_subpass");
    post_subpass.setInputAttachment("position_buffer");
    post_subpass.setInputAttachment("normal_buffer");
    post_subpass.setInputAttachment("color_buffer");
    post_subpass.setOutputAttachment(wen::IMGUI_DOCKING_ATTACHMENT);

    render_pass->addSubpassDependency(
        "main_subpass", "post_subpass",
        {vk::PipelineStageFlagBits::eColorAttachmentOutput,
         vk::PipelineStageFlagBits::eColorAttachmentOutput},
        {vk::AccessFlagBits::eColorAttachmentWrite,
         vk::AccessFlagBits::eColorAttachmentRead});

    render_pass->build();

    renderer = interface->createRenderer(std::move(render_pass));
    imGui = std::make_shared<wen::Imgui>(*renderer, true);

    // shader
    auto main_vert_shader =
        interface->loadShader("deferred_shading/main.vert", wen::ShaderStage::eVertex);
    auto main_frag_shader = interface->loadShader("deferred_shading/main.frag",
                                                  wen::ShaderStage::eFragment);
    main_shader_program_ = interface->createGraphicsShaderProgram();
    main_shader_program_->attach(main_vert_shader).attach(main_frag_shader);
    auto post_vert_shader =
        interface->loadShader("deferred_shading/post.vert", wen::ShaderStage::eVertex);
    auto post_frag_shader = interface->loadShader("deferred_shading/post.frag",
                                                  wen::ShaderStage::eFragment);
    post_shader_program_ = interface->createGraphicsShaderProgram();
    post_shader_program_->attach(post_vert_shader).attach(post_frag_shader);

    // vertex input
    auto vertex_input = interface->createVertexInput({
        {.binding = 0,
         .input_rate = wen::InputRate::eVertex,
         .formats =
         {
         wen::VertexType::eFloat3, // position
         wen::VertexType::eFloat3, // normal
         wen::VertexType::eFloat3  // color
         }},
        {.binding = 1,
         .input_rate = wen::InputRate::eInstance,
         .formats = {
         wen::VertexType::eFloat3 // offset
         }}
    });

    model_ = interface->loadNormalModel("dragon.obj");
    vertex_buffer_ =
        interface->createVertexBuffer(sizeof(wen::Vertex), model_->vertex_count);
    index_buffer_ =
        interface->createIndexBuffer(wen::IndexType::eUint32, model_->index_count);
    model_->upload(vertex_buffer_, index_buffer_);
    n_ = 3;
    int n2 = n_ / 2;
    for (int i = 0; i < n_ * n_ * n_; i++) {
        offsets_.push_back(
            {i % n_ - n2, (i / n_) % n_ - n2, ((i / n_) / n_) % n_ - n2});
    }
    offsets_buffer_ = interface->createVertexBuffer(sizeof(glm::vec3), offsets_.size());
    offsets_buffer_->setData(offsets_);

    // descriptor set
    main_descriptor_set_ = interface->createDescriptorSet();
    main_descriptor_set_->addDescriptors({
        {0, vk::DescriptorType::eUniformBuffer, wen::ShaderStage::eVertex}  // camera
    });
    main_descriptor_set_->build();
    post_descriptor_set_ = interface->createDescriptorSet();
    post_descriptor_set_->addDescriptors({
        {0, vk::DescriptorType::eInputAttachment, 3,
         wen::ShaderStage::eFragment}, // input attachment
        {1, vk::DescriptorType::eUniformBuffer, wen::ShaderStage::eFragment}, // camera
        {2, vk::DescriptorType::eUniformBuffer, wen::ShaderStage::eFragment}  // light
    });
    post_descriptor_set_->build();

    camera_ = std::make_unique<Camera>();
    camera_->setViewportSize(viewport_size.x, viewport_size.y);
    camera_->setInitialState({0.0f, 0.0f, -n_ - 3.0f}, {0.0f, 0.0f, 1.0f});
    main_descriptor_set_->bindUniform(0, camera_->uniform_buffer);
    gbuffer_sampler_ = interface->createSampler();
    refreshInputAttachments();
    post_descriptor_set_->bindUniform(1, camera_->uniform_buffer);
    light_ = std::make_unique<Light>(interface);
    light_->data->lights[0].position = glm::vec3(1.0f, 1.0f, 1.0f);
    light_->data->lights[0].color = glm::vec3(1.0f, 1.0f, 1.0f);
    light_->data->lights[0].intensity = 1.0f;
    light_->data->light_count = 1;
    light_->data->display_mode = 0;
    post_descriptor_set_->bindUniform(2, light_->uniform_buffer);

    // render pipeline
    main_render_pipeline_ =
        interface->createGraphicsRenderPipeline(renderer, main_shader_program_, "main_subpass");
    main_render_pipeline_->setVertexInput(vertex_input);
    main_render_pipeline_->setDescriptorSet(main_descriptor_set_);
    main_render_pipeline_->compile({
        .depth_test_enable = true,
        .dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor}
    });
    post_render_pipeline_ =
        interface->createGraphicsRenderPipeline(renderer, post_shader_program_, "post_subpass");
    post_render_pipeline_->setDescriptorSet(post_descriptor_set_);
    post_render_pipeline_->compile({.depth_test_enable = true});
}

void DeferredShading::update(float ts, float w, float h) {
    camera_->setViewportSize(w, h);
    camera_->upload();
    camera_->update(ts);

    static float time = 0;
    light_->data->lights[0].position = glm::rotateY(glm::vec3(1.0f, 1.0f, 1.0f), time);
    time += 3 * ts;
}

void DeferredShading::render(float w, float h) {
    refreshInputAttachments();
    renderer->setClearColor(wen::IMGUI_DOCKING_ATTACHMENT,
                            {
                                {0.3f, 0.5f, 0.7f, 1.0f}
    });
    renderer->bindPipeline(main_render_pipeline_);
    renderer->bindDescriptorSets(main_render_pipeline_);
    renderer->setViewport(0, h, w, -h);
    renderer->setScissor(0, 0, static_cast<uint32_t>(w), static_cast<uint32_t>(h));
    renderer->bindVertexBuffers({vertex_buffer_, offsets_buffer_});
    renderer->bindIndexBuffer(index_buffer_);
    renderer->drawModel(model_, offsets_.size(), 0);
    renderer->nextSubpass();
    renderer->bindPipeline(post_render_pipeline_);
    renderer->bindDescriptorSets(post_render_pipeline_);
    renderer->drawIndexed(3, 2, 0, 0, 0);
}

void DeferredShading::refreshInputAttachments() {
    if (!post_descriptor_set_ || !renderer || !gbuffer_sampler_) {
        return;
    }

    auto position_index =
        renderer->render_pass->getAttachmentIndex("position_buffer", true);
    auto normal_index =
        renderer->render_pass->getAttachmentIndex("normal_buffer", true);
    auto color_index = renderer->render_pass->getAttachmentIndex("color_buffer", true);

    std::array<vk::ImageView, 3> current_views{
        renderer->framebuffer_set->attachments[position_index]->image_view,
        renderer->framebuffer_set->attachments[normal_index]->image_view,
        renderer->framebuffer_set->attachments[color_index]->image_view,
    };

    if (current_views == last_input_views_) {
        return;
    }

    post_descriptor_set_->bindInputAttachments(
        0, renderer,
        {
            {"position_buffer", gbuffer_sampler_},
            {  "normal_buffer", gbuffer_sampler_},
            {   "color_buffer", gbuffer_sampler_}
    });
    last_input_views_ = current_views;
}

void DeferredShading::imgui() {
    ImGui::Begin("Settings");
    ImGui::Text("(%.1f FPS)", ImGui::GetIO().Framerate);
    ImGui::Separator();
    ImGui::ColorEdit3("light color", glm::value_ptr(light_->data->lights[0].color));
    ImGui::SliderFloat("light intensity", &light_->data->lights[0].intensity, 0.0f,
                       10.0f);
    ImGui::Separator();
    ImGui::Text("model count: %d", n_ * n_ * n_);
    ImGui::Text("vertex count: %d", model_->vertex_count);
    ImGui::Text("index count: %d", model_->index_count);
    ImGui::Text("triangle count: %d", model_->index_count / 3);
    ImGui::Separator();
    ImGui::Text("display mode:");
    const char* items[] = {"final color", "position", "z-buffer", "normal", "color"};
    ImGui::Combo("##display_mode", &light_->data->display_mode, items,
                 IM_ARRAYSIZE(items));
    ImGui::End();
}

void DeferredShading::destroy() {
    camera_.reset();
    light_.reset();
    main_descriptor_set_.reset();
    post_descriptor_set_.reset();
    gbuffer_sampler_.reset();
    main_shader_program_.reset();
    post_shader_program_.reset();
    model_.reset();
    vertex_buffer_.reset();
    index_buffer_.reset();
    offsets_buffer_.reset();
    main_render_pipeline_.reset();
    post_render_pipeline_.reset();
}