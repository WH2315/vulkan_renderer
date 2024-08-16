#pragma once

#include <vulkan/vulkan.hpp>

namespace wen {

class Context final {
public:
    ~Context() = default;

    static void init();
    static Context& instance();
    static void quit();

    void initialize();
    void destroy();

public:
    vk::Instance vk_instance;
    vk::SurfaceKHR surface;

private:
    void createVkInstance();
    void createSurface();

private:
    Context() = default;

    static std::unique_ptr<Context> instance_;
};

} // namespace wen