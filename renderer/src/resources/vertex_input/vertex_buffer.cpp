#include "resources/vertex_input/vertex_buffer.hpp"
#include "manager.hpp"

namespace wen {

VertexBuffer::VertexBuffer(uint32_t size, uint32_t count) {
    staging_ = std::make_unique<Buffer>(
        static_cast<uint64_t>(size) * count,
        vk::BufferUsageFlagBits::eTransferSrc,
        VMA_MEMORY_USAGE_CPU_ONLY,
        0
    );
    buffer_ = std::make_unique<Buffer>(
        static_cast<uint64_t>(size) * count,
        vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eVertexBuffer,
        VMA_MEMORY_USAGE_GPU_ONLY,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
    );
}

VertexBuffer::~VertexBuffer() {
    staging_.reset();
    buffer_.reset();
}

void* VertexBuffer::map() {
    return staging_->map();
}

void  VertexBuffer::flush() {
    auto cmdbuf = manager->command_pool->allocateSingleUse();
    vk::BufferCopy regions;
    regions.setSize(staging_->size)
        .setSrcOffset(0)
        .setDstOffset(0);
    cmdbuf.copyBuffer(staging_->buffer, buffer_->buffer, regions);
    manager->command_pool->freeSingleUse(cmdbuf);
}

void  VertexBuffer::unmap() {
    staging_->unmap();
}

} // namespace wen