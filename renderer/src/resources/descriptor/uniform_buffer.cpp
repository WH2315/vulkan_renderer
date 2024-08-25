#include "resources/descriptor/uniform_buffer.hpp"

namespace wen {

UniformBuffer::UniformBuffer(uint64_t size) {
    buffer_ = std::make_unique<Buffer>(
        size,
        vk::BufferUsageFlagBits::eUniformBuffer,
        VMA_MEMORY_USAGE_AUTO_PREFER_HOST,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );
    buffer_->map();
}

UniformBuffer::~UniformBuffer() {
    buffer_.reset();
}

} // namespace wen