#pragma once

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

namespace wen {

class Buffer {
public:
    Buffer(uint64_t size, vk::BufferUsageFlags buffer_usage, VmaMemoryUsage usage, VmaAllocationCreateFlags flags);
    ~Buffer();

    void* map();
    void unmap();

public:
    vk::Buffer buffer;
    uint64_t size;
    void* data;

private:
    VmaAllocation allocation_;
    bool mapped_;
};

} // namespace wen