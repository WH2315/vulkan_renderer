#include "scenes/ray_marching.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>

RayMarchingInfo::RayMarchingInfo(wen::Interface& interface) {
    uniform_buffer = interface.createUniformBuffer(sizeof(RayMarchingUniform));
    data = static_cast<RayMarchingUniform*>(uniform_buffer->getData());
    memset(data, 0, sizeof(RayMarchingUniform));
}

void RayMarching::initialize() {
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
        interface->loadShader("ray_marching/shader.vert", wen::ShaderStage::eVertex);
    auto frag_shader =
        interface->loadShader("ray_marching/shader.frag", wen::ShaderStage::eFragment);
    shader_program_ = interface->createShaderProgram();
    shader_program_->attach(vert_shader).attach(frag_shader);

    // descriptor set
    auto descriptor_set = interface->createDescriptorSet();
    descriptor_set->addDescriptors({
        {0, vk::DescriptorType::eUniformBuffer,wen::ShaderStage::eFragment        }, // camera
        {1, vk::DescriptorType::eUniformBuffer,
         wen::ShaderStage::eFragment}  // ray marching info
    });
    descriptor_set->build();

    camera_ = std::make_unique<Camera>();
    camera_->setViewportSize(viewport_size.x, viewport_size.y);
    camera_->setInitialState({0.0f, 1.0f, -6.0f}, {0.0f, 0.0f, 1.0f});
    descriptor_set->bindUniform(0, camera_->uniform_buffer);

    info_ = std::make_unique<RayMarchingInfo>(*interface);
    info_->data->max_steps = 100;
    info_->data->max_dist = 100.0f;
    info_->data->epsillon_dist = 0.0001f;
    info_->data->sphere = glm::vec4(0.0f, 1.0f, 2.0f, 1.0f);
    info_->data->intensity = 1.0f;
    descriptor_set->bindUniform(1, info_->uniform_buffer);

    // render pipeline
    render_pipeline_ =
        interface->createRenderPipeline(renderer, shader_program_, "main_subpass");
    render_pipeline_->setDescriptorSet(descriptor_set);
    render_pipeline_->compile({
        .depth_test_enable = true,
        .dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor}
    });
}

void RayMarching::update(float ts, float w, float h) {
    camera_->setViewportSize(w, h);
    camera_->upload();
    camera_->update(ts);

    info_->data->window_size = glm::vec2(w, h);
    static float time = 0.0f;
    time += 2 * ts;
    info_->data->light = glm::vec3(info_->data->sphere.x, 2.0f, info_->data->sphere.z) +
                         glm::rotateY(glm::vec3(1.2f, 1.2f, 1.2f), time);
}

void RayMarching::render(float w, float h) {
    renderer->setClearColor(wen::IMGUI_DOCKING_ATTACHMENT,
                            {
                                {0.3f, 0.8f, 1.0f, 1.0f}
    });
    renderer->bindPipeline(render_pipeline_);
    renderer->bindDescriptorSets(render_pipeline_);
    renderer->setViewport(0, h, w, -h);
    renderer->setScissor(0, 0, static_cast<uint32_t>(w), static_cast<uint32_t>(h));
    renderer->draw(3, 2, 0, 0);
}

void RayMarching::imgui() {
    ImGui::Begin("Settings");
    ImGui::Text("FPS: %f", ImGui::GetIO().Framerate);
    ImGui::SliderInt("max steps", &info_->data->max_steps, 1, 1000);
    ImGui::SliderFloat("max dist", &info_->data->max_dist, 0.001, 1000);
    ImGui::SliderFloat("epsillon dist", &info_->data->epsillon_dist, 0.0001, 0.01);
    ImGui::SliderFloat3("sphere pos", &info_->data->sphere.x, -3, 3);
    ImGui::SliderFloat("sphere size", &info_->data->sphere.w, 0.001, 2);
    ImGui::SliderFloat("intensity", &info_->data->intensity, 0.5, 5);
    ImGui::End();
}

void RayMarching::destroy() {
    info_.reset();
    camera_.reset();
    shader_program_.reset();
    render_pipeline_.reset();
}