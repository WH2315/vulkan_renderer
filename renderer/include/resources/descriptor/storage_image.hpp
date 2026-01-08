#pragma once

#include "resources/image.hpp"
#include "resources/specific_texture.hpp"
#include <memory>

namespace wen {

class StorageImage : public SpecificTexture {
public:
    StorageImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageUsageFlags usage);
    ~StorageImage() override;

    vk::ImageLayout getImageLayout() override { return vk::ImageLayout::eGeneral; }
    vk::ImageView getImageView() override { return image_view_; }
    uint32_t getMipLevels() override { return 1; }
    
private:
    std::unique_ptr<Image> image_;
    vk::ImageView image_view_;
};

} // namespace wen