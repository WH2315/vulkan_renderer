#include "scenes/model_manager.hpp"

void ModelManager::initialize() {
    auto render_pass = interface->createRenderPass(false);
    render_pass->addAttachment(wen::SWAPCHAIN_IMAGE_ATTACHMENT,
                               wen::AttachmentType::eColor);
    render_pass->addAttachment(wen::DEPTH_ATTACHMENT, wen::AttachmentType::eDepth);
    render_pass->addAttachment(wen::IMGUI_DOCKING_ATTACHMENT,
                               wen::AttachmentType::eRGBA8Unorm);

    auto& subpass = render_pass->addSubpass("main_subpass");
    subpass.setOutputAttachment(wen::SWAPCHAIN_IMAGE_ATTACHMENT);
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

    // shader
    auto vert_shader =
        interface->loadShader("model_manager/shader.vert", wen::ShaderStage::eVertex);
    auto frag_shader =
        interface->loadShader("model_manager/shader.frag", wen::ShaderStage::eFragment);
    shader_program_ = interface->createShaderProgram();
    shader_program_->attach(vert_shader).attach(frag_shader);

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
         wen::VertexType::eFloat3, // offset
         wen::VertexType::eFloat   // scale
         }}
    });

    vertex_buffer_ = interface->createVertexBuffer(sizeof(wen::VertexType), 4096000);
    index_buffer_ = interface->createIndexBuffer(wen::IndexType::eUint32, 4096000);

    // descriptor set
    auto descriptor_set = interface->createDescriptorSet();
    descriptor_set->addDescriptors({
        {0, vk::DescriptorType::eUniformBuffer, wen::ShaderStage::eVertex}
    });
    descriptor_set->build();

    camera_ = std::make_unique<Camera>();
    camera_->setInitialState({0.0f, 0.0f, -3.0f}, {0.0f, 0.0f, 1.0f});
    descriptor_set->bindUniform(0, camera_->uniform_buffer);

    // render pipeline
    render_pipeline_ =
        interface->createRenderPipeline(renderer, shader_program_, "main_subpass");
    render_pipeline_->setVertexInput(vertex_input);
    render_pipeline_->setDescriptorSet(descriptor_set);
    render_pipeline_->compile({
        .depth_test_enable = true,
        .dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor}
    });
}

void ModelManager::update(float ts) {
    camera_->update(ts);
}

void ModelManager::render() {
    renderer->setClearColor(wen::SWAPCHAIN_IMAGE_ATTACHMENT,
                            {
                                {0.3f, 0.8f, 1.0f, 1.0f}
    });
    auto width = wen::renderer_config->getWidth(),
         height = wen::renderer_config->getHeight();
    auto w = static_cast<float>(width), h = static_cast<float>(height);
    renderer->bindPipeline(render_pipeline_);
    renderer->bindDescriptorSets(render_pipeline_);
    renderer->setViewport(0, h, w, -h);
    renderer->setScissor(0, 0, w, h);
    renderer->bindVertexBuffer(vertex_buffer_);
    renderer->bindIndexBuffer(index_buffer_);
    for (auto& [filename, info] : models_) {
        info.vertex_buffer->setData(info.inner_infos);
        renderer->bindVertexBuffer(info.vertex_buffer, 1);
        for (auto& [mesh_name, visable] : info.mesh_visibility) {
            if (!visable) {
                continue;
            }
            renderer->drawMesh(info.model->meshes().at(mesh_name),
                               info.inner_infos.size(), 0);
        }
    }
}

void ModelManager::imgui(VkDescriptorSet image) {
    ImGui::Begin("Settings");

    ImGui::Text("(%.1f FPS)", ImGui::GetIO().Framerate);
    ImGui::Separator();

    // 相机复原
    if (ImGui::Button("reset camera")) {
        camera_->reset();
    }

    static const char* filenames[] = {
        "mori_knob.obj", "dragon.obj",    "bunny.obj",    "teapot.obj",
        "Red.obj",       "sportsCar.obj", "nanosuit.obj",
    };
    static int idx = 0;
    ImGui::Combo("model name", &idx, filenames, IM_ARRAYSIZE(filenames));

    static char filename[1024];
    strcpy_s(filename, filenames[idx]);
    static char name[1024] = {0};
    static wen::Offset model_offset = {0, 0};

    ImGui::InputText("instance name", name, 1024);
    if (ImGui::Button("load model")) {
        if (name[0] == '\0') {
            WEN_ERROR("instance name cannot be empty")
        } else {
            if (models_.find(filename) == models_.end()) {
                auto model = interface->loadNormalModel(filename);
                model_offset =
                    model->upload(vertex_buffer_, index_buffer_, model_offset);
                models_.insert(std::make_pair(
                    filename, ModelInfo{model,
                                        interface->createVertexBuffer(
                                            sizeof(ModelInfo::InnerInfo), 512),
                                        {},
                                        {}}));
                WEN_INFO("load model: {}", filename)
            }

            if (querys_.find(name) == querys_.end()) {
                querys_.insert(std::make_pair(
                    name,
                    std::make_pair(filename, models_[filename].inner_infos.size())));
                models_[filename].inner_infos.push_back({
                    .offset = {0.0f, 0.0f, 0.0f},
                      .scale = 1.0f
                });
            } else {
                WEN_ERROR("instance named \"{}\" already exists", name)
            }
        }
    }

    ImGui::SeparatorText("created instance models");
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0, -1.0f});
    static int selected_number = -1;
    int i = 0;
    static std::string selected;

    // Clear stale selection when instances are gone (e.g., after scene switch).
    if (querys_.empty() || (!selected.empty() && !querys_.count(selected))) {
        selected.clear();
        selected_number = -1;
    }

    for (auto& [name, info] : querys_) {
        if (ImGui::RadioButton(name.c_str(), &selected_number, i)) {
            selected = name;
        }
        i += 1;
    }
    ImGui::PopStyleVar();

    ImGui::SeparatorText("selected model instance properties");
    if (!selected.empty() && querys_.count(selected)) {
        auto& query = querys_[selected];
        auto& model_info = models_[query.first];
        if (query.second < model_info.inner_infos.size()) {
            auto& inner_info = model_info.inner_infos[query.second];
            ImGui::SliderFloat3("offset", &inner_info.offset.x, -10, 10);
            ImGui::SliderFloat("scale", &inner_info.scale, 0.0001, 10);
            auto model = model_info.model;
            ImGui::Text("vertex count: %d", model->vertex_count);
            ImGui::Text("index count: %d", model->index_count);
            ImGui::Text("triangle count: %d", model->index_count / 3);
        }
    }

    ImGui::SeparatorText("selected model mesh names");
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, {0, -1.0f});
    if (!selected.empty() && querys_.count(selected)) {
        auto& info = models_[querys_[selected].first];
        std::vector<std::string> mesh_names;
        for (auto& [name, mesh] : info.model->meshes()) {
            mesh_names.push_back(name);
        }
        for (auto& mesh_name : mesh_names) {
            ImGui::Checkbox(mesh_name.c_str(), &info.mesh_visibility[mesh_name]);
        }
        ImGui::Separator();
    }
    ImGui::PopStyleVar();

    ImGui::End();
    ImGui::Image(image, ImGui::GetContentRegionAvail());
    ImGui::Begin("Viewport");

    ImGui::End();
}

void ModelManager::destroy() {
    camera_.reset();
    shader_program_.reset();
    vertex_buffer_.reset();
    index_buffer_.reset();
    render_pipeline_.reset();
    models_.clear();
    querys_.clear();
}