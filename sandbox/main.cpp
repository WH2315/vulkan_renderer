#include <wen.hpp>
#include "camera.hpp"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "core/imgui.hpp"

int main() {
    wen::Manager* manager = new wen::Manager;

    manager->initializeEngine();

    wen::renderer_config->window_info = {"sandbox", 900, 900};
    wen::renderer_config->debug = true;
    wen::renderer_config->app_name = "sandbox";
    wen::renderer_config->engine_name = "wen";
    wen::renderer_config->vsync = true;

    manager->initializeRenderer();

    auto interface = std::make_shared<wen::Interface>("sandbox/resources");

    wen::renderer_config->setSampleCount(vk::SampleCountFlagBits::e64);

    auto render_pass = interface->createRenderPass();

    auto& subpass = render_pass->addSubpass("main_subpass");
    subpass.setOutputAttachment(wen::SWAPCHAIN_IMAGE_ATTACHMENT);
    subpass.setDepthAttachment(wen::DEPTH_ATTACHMENT);

    render_pass->addSubpassDependency(
        wen::EXTERNAL_SUBPASS,
        "main_subpass",
        {
            vk::PipelineStageFlagBits::eColorAttachmentOutput|vk::PipelineStageFlagBits::eLateFragmentTests,
            vk::PipelineStageFlagBits::eColorAttachmentOutput|vk::PipelineStageFlagBits::eLateFragmentTests
        },
        {
            vk::AccessFlagBits::eColorAttachmentWrite|vk::AccessFlagBits::eDepthStencilAttachmentWrite,
            vk::AccessFlagBits::eColorAttachmentWrite|vk::AccessFlagBits::eDepthStencilAttachmentWrite
        }
    );

    render_pass->build();

    auto renderer = interface->createRenderer(std::move(render_pass));
    auto imgui = std::make_shared<wen::Imgui>(*renderer);

    auto cube_model = interface->loadNormalModel("cube.obj");
    auto cube_vb = interface->createVertexBuffer(sizeof(wen::Vertex), cube_model->vertex_count);
    auto cube_ib = interface->createIndexBuffer(wen::IndexType::eUint32, cube_model->index_count);
    cube_model->upload(cube_vb, cube_ib);

    auto sphere_model = interface->loadNormalModel("sphere.obj");
    auto sphere_vb = interface->createVertexBuffer(sizeof(wen::Vertex), sphere_model->vertex_count);
    auto sphere_ib = interface->createIndexBuffer(wen::IndexType::eUint32, sphere_model->index_count);
    sphere_model->upload(sphere_vb, sphere_ib);

    auto vertex_input = interface->createVertexInput({
        {
            .binding = 0,
            .input_rate = wen::InputRate::eVertex,
            .formats = {
                wen::VertexType::eFloat3, // position
                wen::VertexType::eFloat3, // normal
                wen::VertexType::eFloat3  // color
            }
        }
    });

    auto descriptor_set = interface->createDescriptorSet();
    descriptor_set->addDescriptors({
        {0, vk::DescriptorType::eUniformBuffer, wen::ShaderStage::eVertex|wen::ShaderStage::eFragment},
        {1, vk::DescriptorType::eCombinedImageSampler, wen::ShaderStage::eFragment}
    });
    descriptor_set->build();

    struct CameraData {
        alignas(16) glm::vec3 position;
        alignas(16) glm::mat4 view;
        alignas(16) glm::mat4 projection;
    } camera_data;

    auto camera = Camera(wen::g_window->getWindow(), 60.0f, 0.1f, 100.0f);
    camera.setup(glm::vec3(0, 0, -5), glm::vec3(0, 0, 1));

    auto uniform_buffer = interface->createUniformBuffer(sizeof(CameraData));

    auto texture = interface->loadCubemap("cubemap_yokohama_rgba.ktx");
    auto sampler = interface->createSampler({
        .mag_filter = vk::Filter::eLinear,
        .min_filter = vk::Filter::eLinear,
        .address_mode_u = vk::SamplerAddressMode::eClampToEdge,
        .address_mode_v = vk::SamplerAddressMode::eClampToEdge,
        .address_mode_w = vk::SamplerAddressMode::eClampToEdge,
        .max_anisotropy = 1,
        .border_color = vk::BorderColor::eFloatOpaqueWhite,
        .mipmap_mode = vk::SamplerMipmapMode::eLinear,
        .mip_levels = texture->getMipLevels()
    });

    descriptor_set->bindUniform(0, uniform_buffer);
    descriptor_set->bindTexture(1, texture, sampler);

    auto skybox_sp = interface->createGraphicsShaderProgram();
    skybox_sp->attach(interface->loadShader("skybox.vert", wen::ShaderStage::eVertex)).attach(interface->loadShader("skybox.frag", wen::ShaderStage::eFragment));
    auto skybox_rp = interface->createGraphicsRenderPipeline(renderer, skybox_sp, "main_subpass");
    skybox_rp->setVertexInput(vertex_input);
    skybox_rp->setDescriptorSet(descriptor_set);
    skybox_rp->compile({
        .polygon_mode = vk::PolygonMode::eFill,
        .cull_mode = vk::CullModeFlagBits::eFront,
        .front_face = vk::FrontFace::eCounterClockwise,
        .depth_test_enable = false,
        .depth_write_enable = false,
        .depth_compare_op = vk::CompareOp::eLessOrEqual,
        .dynamic_states = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        }
    });

    auto sp = interface->createGraphicsShaderProgram();
    sp->attach(interface->loadShader("shader.vert", wen::ShaderStage::eVertex)).attach(interface->loadShader("shader.frag", wen::ShaderStage::eFragment));
    auto rp = interface->createGraphicsRenderPipeline(renderer, sp, "main_subpass");
    rp->setVertexInput(vertex_input);
    rp->setDescriptorSet(descriptor_set);
    rp->compile({
        .polygon_mode = vk::PolygonMode::eFill,
        .cull_mode = vk::CullModeFlagBits::eBack,
        .front_face = vk::FrontFace::eCounterClockwise,
        .depth_test_enable = true,
        .depth_write_enable = true,
        .depth_compare_op = vk::CompareOp::eLessOrEqual,
        .dynamic_states = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        }
    });

    while (!manager->shouldClose()) {
        manager->pollEvents();

        static auto start = std::chrono::high_resolution_clock::now();
        static float last_time = 0.0f;
        auto current = std::chrono::high_resolution_clock::now();
        auto time = std::chrono::duration<float, std::chrono::seconds::period>(current - start).count();
        float delta_time = time - last_time;
        last_time = time;

        auto width = wen::renderer_config->getWidth(), height = wen::renderer_config->getHeight();
        auto w = static_cast<float>(width), h = static_cast<float>(height);

        renderer->setClearColor(wen::SWAPCHAIN_IMAGE_ATTACHMENT, {{0.5f, 0.5f, 0.5f, 1.0f}});

        camera.resize(width, height);
        camera.update(delta_time);

        camera_data.position = camera.position;
        camera_data.view = camera.view;
        camera_data.projection = camera.projection;
        memcpy(uniform_buffer->getData(), &camera_data, sizeof(CameraData));

        renderer->beginRender();
        renderer->bindPipeline(skybox_rp);
        renderer->bindDescriptorSets(skybox_rp);
        renderer->setViewport(0, h, w, -h);
        renderer->setScissor(0, 0, width, height);
        renderer->bindVertexBuffer(cube_vb);
        renderer->bindIndexBuffer(cube_ib); 
        renderer->drawIndexed(cube_model->index_count, 1, 0, 0, 0);

        renderer->bindPipeline(rp);
        renderer->bindDescriptorSets(rp);
        renderer->setViewport(0, h, w, -h);
        renderer->setScissor(0, 0, width, height);
        renderer->bindVertexBuffer(sphere_vb);
        renderer->bindIndexBuffer(sphere_ib);
        renderer->drawIndexed(sphere_model->index_count, 1, 0, 0, 0);

        imgui->newFrame();
        ImGui::Text("(%.1f FPS)", ImGui::GetIO().Framerate);
        ImGui::Text("Camera Position: (%.2f, %.2f, %.2f)", camera.position.x, camera.position.y, camera.position.z);
        ImGui::Text("Camera Direction: (%.2f, %.2f, %.2f)", camera.direction.x, camera.direction.y, camera.direction.z);
        imgui->renderFrame();

        renderer->endRender();
    }
    renderer->waitIdle();

    imgui.reset();
    sampler.reset();
    texture.reset();
    uniform_buffer.reset();
    descriptor_set.reset();

    rp.reset();
    sp.reset();
    sphere_ib.reset();
    sphere_vb.reset();

    skybox_rp.reset();
    skybox_sp.reset();
    cube_ib.reset();
    cube_vb.reset();

    renderer.reset();
    interface.reset();

    manager->destroyRenderer();
    manager->destroyEngine();

    delete manager;

    return 0;
}