#include "core/window.hpp"
#include "core/log.hpp"

namespace wen {

Window* g_window = nullptr;

Window::Window(const WindowInfo& info) {
    WEN_INFO("Create window:({0}, {1}, {2})", info.title, info.width, info.height)

    glfwSetErrorCallback([](int error, const char* description) {
        WEN_ERROR("GLFW Error ({0}): {1}", error, description)
    });

    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    window_ = glfwCreateWindow(info.width, info.height, info.title.c_str(), nullptr, nullptr);
}

void Window::pollEvents() const {
    glfwPollEvents();
}

bool Window::shouldClose() const {
    return glfwWindowShouldClose(window_);
}

Window::~Window() {
    glfwDestroyWindow(window_);
    window_ = nullptr;
    glfwTerminate();
}

} // namespace wen