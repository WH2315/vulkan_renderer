#pragma once

#include "basic/device.hpp"

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
    std::unique_ptr<Device> device;

private:
    void createVkInstance();
    void createSurface();

private:
    Context() = default;

    static std::unique_ptr<Context> instance_;
};

} // namespace wen