#pragma once

#include "basic/device.hpp"
#include "basic/swapchain.hpp"
#include "basic/command_pool.hpp"
#include "basic/descriptor_pool.hpp"
#include <vk_mem_alloc.h>

namespace wen {

class Context final {
public:
    ~Context() = default;

    static void init();
    static Context& instance();
    static void quit();

    void initialize();
    void destroy();

    void recreateSwapchain();

public:
    vk::Instance vk_instance;
    vk::SurfaceKHR surface;
    std::unique_ptr<Device> device;
    std::unique_ptr<Swapchain> swapchain;
    std::unique_ptr<CommandPool> command_pool;
    std::unique_ptr<DescriptorPool> descriptor_pool;
    VmaAllocator vma_allocator;

private:
    void createVkInstance();
    void createSurface();
    void createVmaAllocator();

private:
    Context() = default;

    static std::unique_ptr<Context> instance_;
};

} // namespace wen