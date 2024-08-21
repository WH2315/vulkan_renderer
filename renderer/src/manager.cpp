#include "manager.hpp"
#include <glslang/Public/ShaderLang.h>

namespace wen {

Context* manager = nullptr;

void Manager::initializeEngine() {
    g_log = new Log("wen");
    renderer_config = new Configuration;
    WEN_INFO("Initialize Engine!")
}

void Manager::initializeRenderer() {
    glslang::InitializeProcess();
    g_window = new Window(renderer_config->window_info);
    Context::init();
    manager = &Context::instance();
    manager->initialize();
}

bool Manager::shouldClose() const {
    return g_window->shouldClose();
}

void Manager::pollEvents() const {
    g_window->pollEvents();
}

void Manager::destroyRenderer() {
    manager->destroy();
    manager = nullptr;
    delete manager;
    Context::quit();
    g_window = nullptr;
    delete g_window;
    glslang::FinalizeProcess();
}

void Manager::destroyEngine() {
    WEN_INFO("Destroy Engine!")
    renderer_config = nullptr;
    delete renderer_config;
    g_log = nullptr;
    delete g_log;
}

} // namespace wen