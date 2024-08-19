#include "wen.hpp"

int main() {
    wen::Manager* manager = new wen::Manager;

    manager->initializeEngine();

    wen::renderer_config->window_info = {"sandbox", 900, 900};
    wen::renderer_config->debug = true;
    wen::renderer_config->app_name = "sandbox";
    wen::renderer_config->engine_name = "wen";
    wen::renderer_config->sync = true;

    manager->initializeRenderer();

    auto interface = std::make_shared<wen::Interface>("sandbox/resources");

    auto render_pass = interface->createRenderPass();
    render_pass->addAttachment("swapchain_image", wen::AttachmentType::eColor);

    auto& subpass = render_pass->addSubpass("main_subpass");
    subpass.setOutputAttachment("swapchain_image");

    render_pass->addSubpassDependency(
        "external_subpass",
        "main_subpass",
        {
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eColorAttachmentOutput
        },
        {
            vk::AccessFlagBits::eNone,
            vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite
        }
    );

    render_pass->build();

    auto renderer = interface->createRenderer(std::move(render_pass));

    auto vert_shader = interface->loadShader("vert.spv", wen::ShaderStage::eVertex);
    auto frag_shader = interface->loadShader("frag.spv", wen::ShaderStage::eFragment);
    auto shader_program = interface->createShaderProgram();
    shader_program->attach(vert_shader).attach(frag_shader);

    auto render_pipeline = interface->createRenderPipeline(renderer, shader_program, "main_subpass");

    render_pipeline->compile({
        .depth_test_enable = false,
        .dynamic_states = {}
    });

    while (!manager->shouldClose()) {
        manager->pollEvents();
    }

    render_pipeline.reset();
    shader_program.reset();
    frag_shader.reset();
    vert_shader.reset();
    renderer.reset();

    interface.reset();

    manager->destroyRenderer();
    manager->destroyEngine();

    delete manager;

    return 0;
}