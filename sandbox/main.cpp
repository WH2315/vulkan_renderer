#include "wen.hpp"

int main() {
    wen::Manager* manager = new wen::Manager;

    manager->initializeEngine();
    manager->initializeRenderer();

    while (!manager->shouldClose()) {
        manager->pollEvents();
    }

    manager->destroyRenderer();
    manager->destroyEngine();

    delete manager;

    return 0;
}