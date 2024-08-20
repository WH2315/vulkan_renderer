#pragma once

#include <vulkan/vulkan.hpp>

namespace wen {

class CommandPool {
public:
    CommandPool(vk::CommandPoolCreateFlags flags);
    ~CommandPool();

    std::vector<vk::CommandBuffer> allocateCommandBuffers(uint32_t count);

private:
    vk::CommandPool command_pool_;
};

} // namespace wen