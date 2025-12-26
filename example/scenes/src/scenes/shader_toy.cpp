#include "scenes/shader_toy.hpp"

ShaderToyInput::ShaderToyInput(wen::Interface& interface) {
    uniform_buffer = interface.createUniformBuffer(sizeof(ShaderToyInput));
    data = static_cast<ShadertoyInputUniform*>(uniform_buffer->getData());
    memset(data, 0, sizeof(ShadertoyInputUniform));
}

void ShaderToy::initialize() {
    auto render_pass = interface->createRenderPass();

    auto& subpass = render_pass->addSubpass("main_subpass");
    subpass.setOutputAttachment(wen::SWAPCHAIN_IMAGE_ATTACHMENT);
    subpass.setDepthAttachment(wen::DEPTH_ATTACHMENT);

    render_pass->build();

    renderer = interface->createRenderer(std::move(render_pass));
    imGui = std::make_shared<wen::Imgui>(*renderer);

    // shader
    auto vert_shader =
        interface->loadShader("shader_toy/shader.vert", wen::ShaderStage::eVertex);
    auto frag_shader =
        interface->loadShader("shader_toy/shader.frag", wen::ShaderStage::eFragment);
    shader_program_ = interface->createShaderProgram();
    shader_program_->attach(vert_shader).attach(frag_shader);

    // vertex input
    auto vertex_input = interface->createVertexInput({
        {.binding = 0,
         .input_rate = wen::InputRate::eVertex,
         .formats = {
             wen::VertexType::eFloat2 // position
         }}
    });

    vertex_buffer_ = interface->createVertexBuffer(sizeof(glm::vec2), 4);
    vertex_buffer_->setData<glm::vec2>({
        { 1.0f,  1.0f},
        {-1.0f,  1.0f},
        { 1.0f, -1.0f},
        {-1.0f, -1.0f},
    });
    index_buffer_ = interface->createIndexBuffer(wen::IndexType::eUint16, 6);
    index_buffer_->setData<uint16_t>({
        0,
        1,
        2,
        2,
        1,
        3,
    });

    // descriptor set
    auto descriptor_set = interface->createDescriptorSet();
    descriptor_set->addDescriptors({
        {0, vk::DescriptorType::eUniformBuffer, wen::ShaderStage::eFragment}
    });
    descriptor_set->build();

    input_ = std::make_unique<ShaderToyInput>(*interface);
    descriptor_set->bindUniform(0, input_->uniform_buffer);

    // push constants
    push_constants_ = interface->createPushConstants(
        wen::ShaderStage::eVertex | wen::ShaderStage::eFragment,
        {
            { "width", wen::ConstantType::eFloat},
            {"height", wen::ConstantType::eFloat}
    });

    // render pipeline
    render_pipeline_ =
        interface->createRenderPipeline(renderer, shader_program_, "main_subpass");
    render_pipeline_->setVertexInput(vertex_input);
    render_pipeline_->setDescriptorSet(descriptor_set);
    render_pipeline_->setPushConstants(push_constants_);
    render_pipeline_->compile({
        .depth_test_enable = true,
        .dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor}
    });
}

void ShaderToy::update(float ts) {
    auto width = wen::renderer_config->getWidth(),
         height = wen::renderer_config->getHeight();
    auto w = static_cast<float>(width), h = static_cast<float>(height);
    push_constants_->pushConstant("width", &w);
    push_constants_->pushConstant("height", &h);

    time_ += ts;
    input_->data->iResolution = glm::vec3(w, h, 1.0f);
    input_->data->iTime = time_;
    input_->data->iTimeDelta = ts;
    input_->data->iFrameRate = ImGui::GetIO().Framerate;
}

void ShaderToy::render() {
    auto width = wen::renderer_config->getWidth(),
         height = wen::renderer_config->getHeight();
    auto w = static_cast<float>(width), h = static_cast<float>(height);
    renderer->bindPipeline(render_pipeline_);
    renderer->bindDescriptorSets(render_pipeline_);
    renderer->pushConstants(render_pipeline_);
    renderer->setViewport(0, h, w, -h);
    renderer->setScissor(0, 0, w, h);
    renderer->bindVertexBuffer(vertex_buffer_);
    renderer->bindIndexBuffer(index_buffer_);
    renderer->drawIndexed(6, 1, 0, 0, 0);
}

void ShaderToy::imgui() {
    ImGui::Text("(%.1f FPS)", ImGui::GetIO().Framerate);
}

void ShaderToy::destroy() {}