#include "scenes/shader_toy.hpp"
#include "scenes/model_manager.hpp"
#include "scenes/ray_marching.hpp"

int main() {
    wen::Manager* manager = new wen::Manager;

    manager->initializeEngine();

    wen::renderer_config->window_info = {"scenes", 1600, 900};
    wen::renderer_config->debug = true;
    wen::renderer_config->app_name = "scenes";
    wen::renderer_config->engine_name = "wen";
    wen::renderer_config->vsync = false;

    manager->initializeRenderer();

    auto interface = std::make_shared<wen::Interface>("example/scenes/resources");

    wen::renderer_config->setSampleCount(vk::SampleCountFlagBits::e64);

    auto scene_manager = std::make_unique<SceneManager>(interface);

    scene_manager->addScene<ShaderToy>("Shader Toy");
    scene_manager->addScene<ModelManager>("Model Manager");
    scene_manager->addScene<RayMarching>("Ray Marching");

    scene_manager->setActiveScene("Shader Toy");

    while (!manager->shouldClose()) {
        manager->pollEvents();
        scene_manager->update();
        scene_manager->render();
    }

    scene_manager.reset();
    interface.reset();

    manager->destroyRenderer();
    manager->destroyEngine();

    delete manager;

    return 0;
}