#pragma once

#include "resources/specific_texture.hpp"
#include <ktxvulkan.h>

namespace wen {

class KtxTexture : public SpecificTexture {
public:
    KtxTexture(const std::string& filename);
    ~KtxTexture() override;

    vk::ImageLayout getImageLayout() override { return vk::ImageLayout::eShaderReadOnlyOptimal; }
    vk::ImageView getImageView() override { return image_view_; }
    uint32_t getMipLevels() override { return vk_texture_.levelCount; }

private:
    ktxTexture* ktx_texture_;
    ktxVulkanTexture vk_texture_;
    vk::ImageView image_view_;
};

} // namespace wen