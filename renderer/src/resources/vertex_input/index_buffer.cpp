#include "resources/vertex_input/index_buffer.hpp"
#include "base/utils.hpp"
#include "manager.hpp"

namespace wen {

IndexBuffer::IndexBuffer(IndexType type, uint32_t count) {
    index_type_ = convert<vk::IndexType>(type);
    staging_ = std::make_unique<Buffer>(
        static_cast<uint64_t>(count) * convert<uint32_t>(type),
        vk::BufferUsageFlagBits::eTransferSrc,
        VMA_MEMORY_USAGE_CPU_ONLY,
        0
    );
    buffer_ = std::make_unique<Buffer>(
        static_cast<uint64_t>(count) * convert<uint32_t>(type),
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
        VMA_MEMORY_USAGE_GPU_ONLY,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
    );
}

IndexBuffer::~IndexBuffer() {
    staging_.reset();
    buffer_.reset();
}

void* IndexBuffer::map() {
    return staging_->map();
}

void IndexBuffer::flush() {
    auto cmdbuf = manager->command_pool->allocateSingleUse();
    vk::BufferCopy regions;
    regions.setSize(staging_->size).setSrcOffset(0).setDstOffset(0);
    cmdbuf.copyBuffer(staging_->buffer, buffer_->buffer, regions);
    manager->command_pool->freeSingleUse(cmdbuf);
}

void IndexBuffer::unmap() {
    staging_->unmap();
}

} // namespace wen