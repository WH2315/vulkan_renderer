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

vk::CommandBuffer CommandPool::allocateSingleUse() {
    vk::CommandBufferAllocateInfo allocate_info;
    allocate_info.setCommandPool(command_pool_)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(1);

    auto cmdbuf = manager->device->device.allocateCommandBuffers(allocate_info)[0];
    vk::CommandBufferBeginInfo begin_info;
    begin_info.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    cmdbuf.begin(begin_info);

    return std::move(cmdbuf);
}

void CommandPool::freeSingleUse(vk::CommandBuffer cmdbuf) {
    cmdbuf.end();

    vk::SubmitInfo submits = {};
    submits.setCommandBuffers(cmdbuf);
    manager->device->transfer_queue.submit(submits, nullptr);
    manager->device->transfer_queue.waitIdle();
    manager->device->device.freeCommandBuffers(command_pool_, cmdbuf);
}

} // namespace wen