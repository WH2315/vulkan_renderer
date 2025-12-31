#include "scenes/pbr_scene.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

PBRMaterial::PBRMaterial(wen::Interface& interface) {
    uniform_buffer = interface.createUniformBuffer(sizeof(MaterialUniform));
    data = static_cast<MaterialUniform*>(uniform_buffer->getData());
    memset(data, 0, sizeof(MaterialUniform));
}

PBRScene::Light::Light(wen::Interface& interface) {
    uniform_buffer = interface.createUniformBuffer(sizeof(LightUniform));
    data = static_cast<LightUniform*>(uniform_buffer->getData());
    memset(data, 0, sizeof(LightUniform));
}

void PBRScene::initialize() {
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

    // shader
    auto vert_shader =
        interface->loadShader("pbr_scene/shader.vert", wen::ShaderStage::eVertex);
    auto frag_shader =
        interface->loadShader("pbr_scene/shader.frag", wen::ShaderStage::eFragment);
    shader_program_ = interface->createGraphicsShaderProgram();
    shader_program_->attach(vert_shader).attach(frag_shader);

    // vertex input
    auto vertex_input = interface->createVertexInput({
        {.binding = 0,
         .input_rate = wen::InputRate::eVertex,
         .formats = {
             wen::VertexType::eFloat3, // position
             wen::VertexType::eFloat3, // normal
             wen::VertexType::eFloat3  // color
         }}
    });

    // model
    model_ = interface->loadNormalModel("mori_knob.obj");
    vertex_buffer_ =
        interface->createVertexBuffer(sizeof(wen::Vertex), model_->vertex_count);
    index_buffer_ =
        interface->createIndexBuffer(wen::IndexType::eUint32, model_->index_count);
    model_->upload(vertex_buffer_, index_buffer_);

    // descriptor set
    auto descriptor_set = interface->createDescriptorSet();
    descriptor_set->addDescriptors({
        {0, vk::DescriptorType::eUniformBuffer,
         wen::ShaderStage::eVertex | wen::ShaderStage::eFragment           }, // camera
        {1, vk::DescriptorType::eUniformBuffer,
         wen::ShaderStage::eFragment                                       }, // material
        {2, vk::DescriptorType::eUniformBuffer, wen::ShaderStage::eFragment}, // light
    });
    descriptor_set->build();

    // camera
    camera_ = std::make_unique<Camera>();
    camera_->setViewportSize(viewport_size.x, viewport_size.y);
    camera_->setInitialState({0.0f, 0.0f, -3.0f}, {0.0f, 0.0f, 1.0f});
    descriptor_set->bindUniform(0, camera_->uniform_buffer);
    // material
    material_ = std::make_unique<PBRMaterial>(*interface);
    material_->data->albedo = glm::vec3(0.5f);
    material_->data->metallic = 0.7f;
    material_->data->roughness = 0.5f;
    material_->data->ao = 0.7f;
    descriptor_set->bindUniform(1, material_->uniform_buffer);
    // light
    light_ = std::make_unique<Light>(*interface);
    light_->data->color = glm::vec3(1.0f, 1.0f, 1.0f);
    light_->data->direction = glm::normalize(glm::vec3(0, -1, 0));
    descriptor_set->bindUniform(2, light_->uniform_buffer);

    // render pipeline
    render_pipeline_ =
        interface->createGraphicsRenderPipeline(renderer, shader_program_, "main_subpass");
    render_pipeline_->setVertexInput(vertex_input);
    render_pipeline_->setDescriptorSet(descriptor_set);
    render_pipeline_->compile({
        .depth_test_enable = true,
        .dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor}
    });
}

void PBRScene::update(float ts, float w, float h) {
    camera_->setViewportSize(w, h);
    camera_->upload();
    camera_->update(ts);

    static float time = 0;
    time += ts;
    float n = 1.0f;
    auto pos = glm::rotateY(glm::vec3(n, 0.0f, n), time);
    light_->data->point_lights[0].position = glm::vec3(0, pos.x, pos.z); // 绕X轴旋转
    light_->data->point_lights[1].position = glm::vec3(pos.x, 0, pos.z); // 绕Y轴旋转
    light_->data->point_lights[2].position = glm::vec3(pos.x, pos.z, 0); // 绕Z轴旋转
}

void PBRScene::render(float w, float h) {
    renderer->bindPipeline(render_pipeline_);
    renderer->bindDescriptorSets(render_pipeline_);
    renderer->setViewport(0, h, w, -h);
    renderer->setScissor(0, 0, static_cast<uint32_t>(w), static_cast<uint32_t>(h));
    renderer->bindVertexBuffer(vertex_buffer_);
    renderer->bindIndexBuffer(index_buffer_);
    renderer->drawModel(model_, 1, 0);
}

void PBRScene::imgui() {
    ImGui::Begin("Settings");
    ImGui::Text("FPS: %f", ImGui::GetIO().Framerate);
    ImGui::ColorEdit3("LightColor", &light_->data->color.r);
    ImGui::ColorEdit3("PointLight0", &light_->data->point_lights[0].color.r);
    ImGui::ColorEdit3("PointLight1", &light_->data->point_lights[1].color.r);
    ImGui::ColorEdit3("PointLight2", &light_->data->point_lights[2].color.r);
    ImGui::Separator();
    ImGui::ColorEdit3("Albedo", &material_->data->albedo.r);
    ImGui::SliderFloat("Metallic", &material_->data->metallic, 0, 1);
    ImGui::SliderFloat("Roughness", &material_->data->roughness, 0, 1);
    ImGui::SliderFloat("AO", &material_->data->ao, 0, 1);
    ImGui::End();
}

void PBRScene::destroy() {
    camera_.reset();
    light_.reset();
    material_.reset();
    shader_program_.reset();
    model_.reset();
    vertex_buffer_.reset();
    index_buffer_.reset();
    render_pipeline_.reset();
}