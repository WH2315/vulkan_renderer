#include "manager.hpp"

namespace wen {

void Manager::initializeEngine() {
    g_log = new Log("wen");
    WEN_INFO("Initialize Engine!")
}

void Manager::initializeRenderer() {
    g_window = new Window({"wen", 1600, 900});
}

bool Manager::shouldClose() const {
    return g_window->shouldClose();
}

void Manager::pollEvents() const {
    g_window->pollEvents();
}

void Manager::destroyRenderer() {
    g_window = nullptr;
    delete g_window;
}

void Manager::destroyEngine() {
    WEN_INFO("Destroy Engine!")
    g_log = nullptr;
    delete g_log;
}

} // namespace wen