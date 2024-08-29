#pragma once

#include "resources/specific_texture.hpp"

namespace wen {

class DataTexture : public SpecificTexture {
public:
    DataTexture(const uint8_t* data, uint32_t width, uint32_t height, uint32_t mip_levels);
    ~DataTexture() override;

    vk::ImageLayout getImageLayout() override { return vk::ImageLayout::eShaderReadOnlyOptimal; }
    vk::ImageView getImageView() override { return image_view_; }
    uint32_t getMipLevels() override { return mip_levels_; }

private:
    std::unique_ptr<Image> image_;
    vk::ImageView image_view_;
    uint32_t mip_levels_;
};

} // namespace wen