#include "wen.hpp"

int main() {
    wen::Manager* manager = new wen::Manager;

    manager->initializeEngine();

    wen::renderer_config->window_info = {"sandbox", 900, 900};
    wen::renderer_config->debug = true;
    wen::renderer_config->app_name = "sandbox";
    wen::renderer_config->engine_name = "wen";

    manager->initializeRenderer();

    while (!manager->shouldClose()) {
        manager->pollEvents();
    }

    manager->destroyRenderer();
    manager->destroyEngine();

    delete manager;

    return 0;
}