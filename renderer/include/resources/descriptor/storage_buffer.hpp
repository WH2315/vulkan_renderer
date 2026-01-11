#pragma once

#include "resources/specific_buffer.hpp"
#include <memory>

namespace wen {

class StorageBuffer : public SpecificBuffer {
public:
    StorageBuffer(uint64_t size, vk::BufferUsageFlags usage_flags,
                  VmaMemoryUsage memory_usage,
                  VmaAllocationCreateFlags allocation_flags);
    ~StorageBuffer() override;

    void* map();
    void flush(vk::DeviceSize size, const vk::Buffer& buffer);
    void unmap();

    vk::Buffer getBuffer() override { return buffer_->buffer; }
    uint64_t getSize() override { return buffer_->size; }
    void* getData() override { return buffer_->data; }

private:
    std::unique_ptr<Buffer> buffer_;
};

} // namespace wen