#include "resources/buffer.hpp"
#include "manager.hpp"

namespace wen {

Buffer::Buffer(uint64_t size, vk::BufferUsageFlags buffer_usage, VmaMemoryUsage usage, VmaAllocationCreateFlags flags) 
    : size(size), data(nullptr), mapped_(false) {
    VmaAllocationCreateInfo allocation_create_info = {};
    allocation_create_info.usage = usage;
    allocation_create_info.flags = flags;

    vk::BufferCreateInfo buffer_create_info = {};
    buffer_create_info.setSize(size)
        .setUsage(buffer_usage);
    
    vmaCreateBuffer(
        manager->vma_allocator,
        reinterpret_cast<VkBufferCreateInfo*>(&buffer_create_info),
        &allocation_create_info,
        reinterpret_cast<VkBuffer*>(&buffer),
        &allocation_,
        nullptr
    );
}

void* Buffer::map() {
    if (mapped_) {
        return data;
    }
    vmaMapMemory(manager->vma_allocator, allocation_, &data);
    mapped_ = true;
    return data;
}

void Buffer::unmap() {
    if (!mapped_) {
        return;
    }
    vmaUnmapMemory(manager->vma_allocator, allocation_);
    data = nullptr;
    mapped_ = false;
}

Buffer::~Buffer() {
    unmap();
    vmaDestroyBuffer(manager->vma_allocator, buffer, allocation_);
}

} // namespace wen