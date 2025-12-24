#pragma once

#include "resources/image.hpp"

namespace wen {

class SpecificTexture {
public:
    SpecificTexture() = default;
    virtual ~SpecificTexture() = default;
    virtual vk::ImageLayout getImageLayout() = 0;
    virtual vk::ImageView getImageView() = 0;
    virtual uint32_t getMipLevels() = 0;
};

} // namespace wen