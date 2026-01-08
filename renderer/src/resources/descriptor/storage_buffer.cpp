#include "resources/descriptor/storage_buffer.hpp"
#include "manager.hpp"

namespace wen {

StorageBuffer::StorageBuffer(uint64_t size, vk::BufferUsageFlags usage_flags,
                             VmaMemoryUsage memory_usage,
                             VmaAllocationCreateFlags allocation_flags) {
    buffer_ = std::make_unique<Buffer>(
        size, usage_flags | vk::BufferUsageFlagBits::eStorageBuffer, memory_usage,
        allocation_flags);
}

void* StorageBuffer::map() {
    return buffer_->map();
}

void StorageBuffer::flush(vk::DeviceSize size, const vk::Buffer& buffer) {
    auto cmdbuf = manager->command_pool->allocateSingleUse();
    vk::BufferCopy copy_region{};
    copy_region.setSize(size).setSrcOffset(0).setDstOffset(0);
    cmdbuf.copyBuffer(buffer_->buffer, buffer, copy_region);
    manager->command_pool->freeSingleUse(cmdbuf);
}

void StorageBuffer::unmap() {
    buffer_->unmap();
}

StorageBuffer::~StorageBuffer() {
    buffer_.reset();
}

} // namespace wen