#pragma once

#include "resources/specific_buffer.hpp"

namespace wen {

class UniformBuffer : public SpecificBuffer {
public:
    UniformBuffer(uint64_t size);
    ~UniformBuffer() override;

    vk::Buffer getBuffer() override { return buffer_->buffer; }
    uint64_t getSize() override { return buffer_->size; }
    void* getData() override { return buffer_->data; };

private:
    std::unique_ptr<Buffer> buffer_;
};

} // namespace wen