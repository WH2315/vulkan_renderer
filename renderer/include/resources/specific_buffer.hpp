#pragma once

#include "resources/buffer.hpp"

namespace wen {

class SpecificBuffer {
public:
    SpecificBuffer() = default;
    virtual ~SpecificBuffer() = default;
    virtual vk::Buffer getBuffer() = 0;
    virtual uint64_t getSize() = 0;
    virtual void* getData() = 0;
};

} // namespace wen