#pragma once

#include "resources/specific_texture.hpp"

namespace wen {

class CubemapTexture : public SpecificTexture {
public:
    CubemapTexture(const std::string& filename);
    ~CubemapTexture() override;

    vk::ImageLayout getImageLayout() override { return vk::ImageLayout::eShaderReadOnlyOptimal; }
    vk::ImageView getImageView() override { return image_view_; }
    uint32_t getMipLevels() override { return mip_levels_; }

private:
    std::unique_ptr<Image> image_;
    vk::ImageView image_view_;
    uint32_t mip_levels_;
};

} // namespace wen