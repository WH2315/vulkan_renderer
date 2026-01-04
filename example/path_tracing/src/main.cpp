#include "ray_tracing.hpp"

int main() {
    wen::Manager* manager = new wen::Manager;

    manager->initializeEngine();

    wen::renderer_config->window_info = {"ray_tracing", 1600, 900};
    wen::renderer_config->app_name = "ray_tracing";
    wen::renderer_config->engine_name = "wen";

    manager->initializeRenderer();

    auto app = new Application();
    app->pushLayer<RayTracing>();
    app->init();
    app->run();
    delete app;

    manager->destroyRenderer();
    manager->destroyEngine();

    delete manager;

    return 0;
}