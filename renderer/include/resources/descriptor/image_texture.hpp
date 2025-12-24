#pragma once

#include "resources/descriptor/data_texture.hpp"

namespace wen {

class ImageTexture : public SpecificTexture {
public:
    ImageTexture(const std::string& filename, uint32_t mip_levels);
    ~ImageTexture() override;

    vk::ImageLayout getImageLayout() override { return vk::ImageLayout::eShaderReadOnlyOptimal; }
    vk::ImageView getImageView() override { return texture_->getImageView(); }
    uint32_t getMipLevels() override { return texture_->getMipLevels(); }

private:
    std::unique_ptr<DataTexture> texture_;
};

} // namespace wen