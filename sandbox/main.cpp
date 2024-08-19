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

    auto vert_shader = interface->loadShader("vert.spv", wen::ShaderStage::eVertex);
    auto frag_shader = interface->loadShader("frag.spv", wen::ShaderStage::eFragment);
    auto shader_program = interface->createShaderProgram();
    shader_program->attach(vert_shader).attach(frag_shader);

    auto render_pipeline = interface->createRenderPipeline(shader_program);

    render_pipeline->compile();

    while (!manager->shouldClose()) {
        manager->pollEvents();
    }

    render_pipeline.reset();
    shader_program.reset();
    frag_shader.reset();
    vert_shader.reset();

    interface.reset();

    manager->destroyRenderer();
    manager->destroyEngine();

    delete manager;

    return 0;
}