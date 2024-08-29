#include "resources/descriptor/image_texture.hpp"
#include "core/log.hpp"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace wen {

ImageTexture::ImageTexture(const std::string& filename, uint32_t mip_levels) {
    stbi_set_flip_vertically_on_load(true);
    int width, height, channels;
    stbi_uc* pixels = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!pixels) {
        WEN_ERROR("Failed to load image: {}", filename)
        return;
    }
    texture_ = std::make_unique<DataTexture>(pixels, width, height, mip_levels);
    stbi_image_free(pixels);
}

ImageTexture::~ImageTexture() {
    texture_.reset();
}

} // namespace wen