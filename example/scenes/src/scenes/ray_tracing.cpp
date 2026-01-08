#include "scenes/ray_tracing.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>
#include <random>

void RayTracing::initialize() {
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

    createAccelerationStructure();

    shader_descriptor_set_ = interface->createDescriptorSet();
    shader_descriptor_set_->addDescriptors({
        {0, vk::DescriptorType::eUniformBuffer,
         wen::ShaderStage::eFragment | wen::ShaderStage::eMiss}, // info
        {1, vk::DescriptorType::eUniformBuffer,
         wen::ShaderStage::eVertex | wen::ShaderStage::eRaygen}  // camera
    });
    shader_descriptor_set_->build();

    // info
    info_uniform_ = interface->createUniformBuffer(sizeof(Info));
    info_ = static_cast<Info*>(info_uniform_->getData());
    info_->clear_color = glm::vec3(0.8f, 0.8f, 0.9f);
    shader_descriptor_set_->bindUniform(0, info_uniform_);
    // camera
    camera_ = std::make_unique<Camera>();
    camera_->setViewportSize(viewport_size.x, viewport_size.y);
    camera_->setInitialState({0.0f, 1.0f, -3.5f}, {0.0f, 0.0f, 1.0f});
    shader_descriptor_set_->bindUniform(1, camera_->uniform_buffer);
    // point light
    push_constants_ = interface->createPushConstants(
        wen::ShaderStage::eFragment | wen::ShaderStage::eRaygen |
            wen::ShaderStage::eClosestHit,
        {
            {    "position", wen::ConstantType::eFloat3}, // 12 -> 16
            {       "color", wen::ConstantType::eFloat3}, // 12 -> 16
            {   "intensity",  wen::ConstantType::eFloat}, // 4 -> 4
            {"sample_count",  wen::ConstantType::eInt32}, // 4 -> 4
    });
    point_light_position_ = glm::vec3(2.0f, 2.0f, 2.0f);

    // ray tracing
    auto raygen =
        interface->loadShader("ray_tracing/raytrace.rgen", wen::ShaderStage::eRaygen);
    auto miss =
        interface->loadShader("ray_tracing/raytrace.rmiss", wen::ShaderStage::eMiss);
    auto closest = interface->loadShader("ray_tracing/raytrace.rchit",
                                         wen::ShaderStage::eClosestHit);
    auto shadow =
        interface->loadShader("ray_tracing/shadow.rmiss", wen::ShaderStage::eMiss);
    ray_tracing_shader_program_ = interface->createRayTracingShaderProgram();
    ray_tracing_shader_program_->setRaygenShader(raygen);
    ray_tracing_shader_program_->setMissShader(miss);
    ray_tracing_shader_program_->setHitGroup({closest, std::nullopt});
    ray_tracing_shader_program_->setMissShader(shadow);
    ray_tracing_descriptor_set_ = interface->createDescriptorSet();
    ray_tracing_descriptor_set_->addDescriptors({
        {0, vk::DescriptorType::eAccelerationStructureKHR,
         wen::ShaderStage::eRaygen | wen::ShaderStage::eClosestHit                      },
        {1,             vk::DescriptorType::eStorageImage,     wen::ShaderStage::eRaygen},
        {2,            vk::DescriptorType::eStorageBuffer, wen::ShaderStage::eClosestHit}
    });
    ray_tracing_descriptor_set_->build();
    image_ = interface->createStorageImage(viewport_size.x, viewport_size.y,
                                           vk::Format::eR32G32B32A32Sfloat,
                                           vk::ImageUsageFlagBits::eSampled);
    ray_tracing_descriptor_set_->bindAccelerationStructure(0, ray_tracing_instance_);
    ray_tracing_descriptor_set_->bindStorageImage(1, image_);
    ray_tracing_descriptor_set_->bindStorageBuffer(
        2, ray_tracing_instance_->getInstanceAddressBuffer());
    ray_tracing_render_pipeline_ =
        interface->createRayTracingRenderPipeline(ray_tracing_shader_program_);
    ray_tracing_render_pipeline_->setDescriptorSet(ray_tracing_descriptor_set_, 0);
    ray_tracing_render_pipeline_->setDescriptorSet(shader_descriptor_set_, 1);
    ray_tracing_render_pipeline_->setPushConstants(push_constants_);
    ray_tracing_render_pipeline_->compile({.max_ray_recursion_depth = 2});

    // graphics pipeline for displaying ray tracing result
    auto vs =
        interface->loadShader("ray_tracing/shader.vert", wen::ShaderStage::eVertex);
    auto fs =
        interface->loadShader("ray_tracing/shader.frag", wen::ShaderStage::eFragment);
    graphics_shader_program_ = interface->createGraphicsShaderProgram();
    graphics_shader_program_->attach(vs).attach(fs);
    image_descriptor_set_ = interface->createDescriptorSet();
    image_descriptor_set_->addDescriptors({
        {0, vk::DescriptorType::eCombinedImageSampler, wen::ShaderStage::eFragment}
    });
    image_descriptor_set_->build();
    sampler_ = interface->createSampler();
    image_descriptor_set_->bindTexture(0, image_, sampler_);
    graphics_render_pipeline_ = interface->createGraphicsRenderPipeline(
        renderer, graphics_shader_program_, "main_subpass");
    graphics_render_pipeline_->setDescriptorSet(image_descriptor_set_, 0);
    graphics_render_pipeline_->setDescriptorSet(shader_descriptor_set_, 1);
    graphics_render_pipeline_->compile({
        .depth_test_enable = true,
        .dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor}
    });

    // no ray tracing
    shader_program_ = interface->createGraphicsShaderProgram();
    shader_program_
        ->attach(
            interface->loadShader("ray_tracing/raster.vert", wen::ShaderStage::eVertex))
        .attach(interface->loadShader("ray_tracing/raster.frag",
                                      wen::ShaderStage::eFragment));
    render_pipeline_ = interface->createGraphicsRenderPipeline(
        renderer, shader_program_, "main_subpass");
    render_pipeline_->setVertexInput(interface->createVertexInput({
        {.binding = 0,
         .input_rate = wen::InputRate::eVertex,
         .formats =
             {
                 wen::VertexType::eFloat3, // position
                 wen::VertexType::eFloat3, // normal
                 wen::VertexType::eFloat3, // color
             }},
    }));
    render_pipeline_->setDescriptorSet(shader_descriptor_set_);
    render_pipeline_->setPushConstants(push_constants_);
    render_pipeline_->compile({
        .depth_test_enable = true,
        .dynamic_states = {vk::DynamicState::eViewport, vk::DynamicState::eScissor}
    });

    last_w = viewport_size.x, last_h = viewport_size.y;
}

void RayTracing::recreateRayTracingOutput() {
    image_ = interface->createStorageImage(viewport_size.x, viewport_size.y,
                                           vk::Format::eR32G32B32A32Sfloat,
                                           vk::ImageUsageFlagBits::eSampled);
    ray_tracing_descriptor_set_->bindStorageImage(1, image_);
    image_descriptor_set_->bindTexture(0, image_, sampler_);
}

void RayTracing::update(float ts, float w, float h) {
    camera_->setViewportSize(w, h);
    camera_->upload();
    camera_->update(ts);

    if (last_w != w || last_h != h) {
        recreateRayTracingOutput();
        last_w = w;
        last_h = h;
    }

    if (light_rotation_enabled_) {
        light_rotation_time_ += ts;
        point_light_position_ = glm::rotateY(
            glm::vec3(2.0f, point_light_position_.y, 2.0f), light_rotation_time_);
    }

    if (is_enable_ray_tracing) {
        static float time = 0;
        time += ts;
        ray_tracing_instance_->update(
            1, [&](uint32_t index, auto updateTransform, auto) {
                auto [position, rotate_axis, rotate_angle, scale] =
                    transform_infos_.at(1)[index];
                rotate_angle += time;
                scale *= (cos(time * 1.8f + rotate_angle) + 3.0f) / 3.0f;
                // 开普勒第三定律
                auto distance3 = glm::pow(glm::length(position), 3.0f);
                auto k = 2.0f / distance3;
                auto w = glm::sqrt(k);
                position = glm::rotate(w * time, rotate_axis) * glm::vec4(position, 1);
                auto transform = glm::translate(position) *
                                 glm::rotate(rotate_angle, rotate_axis) *
                                 glm::scale(glm::mat4(1.0f), glm::vec3(scale));
                updateTransform(transform);
            });
        ray_tracing_instance_->update(
            2, [&](uint32_t index, auto updateTransform, auto) {
                auto [position, rotate_axis, rotate_angle, scale] =
                    transform_infos_.at(2)[index];
                auto transform = glm::translate(position) *
                                 glm::rotate(rotate_angle, rotate_axis) *
                                 glm::scale(glm::mat4(1.0f), glm::vec3(scale));
                updateTransform(transform);
            });
    }
}

void RayTracing::render(float w, float h) {
    renderer->setClearColor(
        wen::IMGUI_DOCKING_ATTACHMENT,
        {
            {info_->clear_color.r, info_->clear_color.g, info_->clear_color.b, 1.0f}
    });
    if (is_enable_ray_tracing) {
        info_->window_size = glm::vec2(w, h);
        renderer->bindPipeline(ray_tracing_render_pipeline_);
        renderer->bindDescriptorSets(ray_tracing_render_pipeline_);
        renderer->pushConstants(ray_tracing_render_pipeline_);
        renderer->traceRays(ray_tracing_render_pipeline_, w, h, 1);
        renderer->beginRenderPass();
        renderer->bindPipeline(graphics_render_pipeline_);
        renderer->bindDescriptorSets(graphics_render_pipeline_);
        renderer->setViewport(0, h, w, -h);
        renderer->setScissor(0, 0, static_cast<uint32_t>(w), static_cast<uint32_t>(h));
        renderer->draw(3, 2, 0, 0);
    } else {
        renderer->bindPipeline(render_pipeline_);
        renderer->bindDescriptorSets(render_pipeline_);
        renderer->pushConstants(render_pipeline_);
        renderer->setViewport(0, h, w, -h);
        renderer->setScissor(0, 0, static_cast<uint32_t>(w), static_cast<uint32_t>(h));
        renderer->bindVertexBuffer(vertex_buffer_);
        renderer->bindIndexBuffer(index_buffer_);
        renderer->drawModel(model1_, 1, 0);
        renderer->drawModel(model2_, 1, 0);
    }
}

void RayTracing::imgui() {
    ImGui::Begin("Settings");
    ImGui::Text("FPS: %f", ImGui::GetIO().Framerate);

    bool changed = false;

    ImGui::Checkbox("Enable Ray Tracing", &is_enable_ray_tracing);
    ImGui::ColorEdit3("Clear Color", &info_->clear_color.r);

    ImGui::SeparatorText("Light");
    changed |= ImGui::SliderFloat("Light Height", &point_light_position_.y, -1, 6);
    push_constants_->pushConstant("position", &point_light_position_);

    if (ImGui::Button(light_rotation_enabled_ ? "Stop Light Rotation"
                                              : "Start Light Rotation")) {
        light_rotation_enabled_ = !light_rotation_enabled_;
    }

    static glm::vec3 color = {1.0f, 1.0f, 1.0f};
    changed |= ImGui::ColorEdit3("Light Color", &color.r);
    push_constants_->pushConstant("color", &color);
    static float intensity = 5.0f;
    changed |= ImGui::SliderFloat("Light Intensity", &intensity, 0, 30);
    push_constants_->pushConstant("intensity", &intensity);

    static int sample_count = 1;
    changed |= ImGui::SliderInt("Sample Count", &sample_count, 1, 9);
    push_constants_->pushConstant("sample_count", &sample_count);

    ImGui::End();
}

void RayTracing::destroy() {}

void RayTracing::createAccelerationStructure() {
    model1_ = interface->loadNormalModel("ray_tracing/wuson.obj");
    model2_ = interface->loadNormalModel("ray_tracing/plane.obj");
    model3_ = interface->loadNormalModel("dragon.obj");
    vertex_buffer_ = interface->createVertexBuffer(sizeof(wen::Vertex), 4096000);
    index_buffer_ = interface->createIndexBuffer(wen::IndexType::eUint32, 4096000);

    auto offset1 = model1_->upload(vertex_buffer_, index_buffer_);
    auto offset2 = model2_->upload(vertex_buffer_, index_buffer_, offset1);

    auto as = interface->createAccelerationStructure();
    as->addModel(model1_);
    as->addModel(model2_);
    as->addModel(model3_);
    as->build(false, false);
    as.reset();

    ray_tracing_instance_ = interface->createRayTracingInstance();
    ray_tracing_instance_->addModel(0, 0, model1_, glm::mat4(1.0f));
    ray_tracing_instance_->addModel(0, 0, model2_, glm::mat4(1.0f));

    std::random_device device;
    std::mt19937 generator(device());
    std::normal_distribution<float> distribution(0, 1);
    std::normal_distribution<float> scaleDistribution(0.3f, 0.2f);
    for (int i = 0; i < 500; i++) {
        auto position =
            glm::vec3(distribution(generator) * 2, distribution(generator) * 2,
                      distribution(generator) * 2);
        auto rotate_axis = glm::vec3(distribution(generator), distribution(generator),
                                     distribution(generator));
        auto rotate_angle = distribution(generator) * 3.1415926535f;
        auto scale = scaleDistribution(generator);
        transform_infos_[1].emplace_back(position, rotate_axis, rotate_angle, scale);
        auto transform = glm::translate(position) *
                         glm::rotate(rotate_angle, rotate_axis) *
                         glm::scale(glm::mat4(1.0f), glm::vec3(scale));
        ray_tracing_instance_->addModel(1, 0, model3_, transform);
    }
    ray_tracing_instance_->addModel(2, 0, model1_, glm::mat4(1.0f));
    transform_infos_[2].emplace_back(glm::vec3(2.0f, 0.0f, 0.0f),
                                     glm::vec3(0.0f, 1.0f, 0.0f), 0.0f, 1.0f);

    ray_tracing_instance_->build(true);
}