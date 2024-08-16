#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <string>

namespace wen {

struct WindowInfo {
    std::string title;
    uint32_t width, height;
};

class Window final {
public:
    Window(const WindowInfo& info);
    ~Window();

    bool shouldClose() const;
    void pollEvents() const;

    auto getWindow() const { return window_; }

private:
    GLFWwindow* window_;
};

extern Window* g_window;

} // namespace wen