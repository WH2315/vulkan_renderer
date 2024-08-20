#include "basic/command_pool.hpp"
#include "manager.hpp"

namespace wen {

CommandPool::CommandPool(vk::CommandPoolCreateFlags flags) {
    vk::CommandPoolCreateInfo create_info;
    create_info.setFlags(flags)
        .setQueueFamilyIndex(manager->device->graphics_queue_family);
    command_pool_ = manager->device->device.createCommandPool(create_info);
}

CommandPool::~CommandPool() {
    manager->device->device.destroyCommandPool(command_pool_);
}

std::vector<vk::CommandBuffer> CommandPool::allocateCommandBuffers(uint32_t count) {
    vk::CommandBufferAllocateInfo allocate_info;
    allocate_info.setCommandPool(command_pool_)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(count);
    return std::move(manager->device->device.allocateCommandBuffers(allocate_info));
}

} // namespace wen