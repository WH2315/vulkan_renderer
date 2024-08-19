#include "base/utils.hpp"
#include "manager.hpp"
#include <fstream>

namespace wen {

vk::ImageView createImageView(vk::Image image, vk::Format format,
                              vk::ImageAspectFlags aspect, uint32_t level_count,
                              uint32_t layer_count, vk::ImageViewType view_type) {
    vk::ImageViewCreateInfo create_info;

    vk::ImageSubresourceRange subresource_range;
    subresource_range.setAspectMask(aspect)
        .setBaseMipLevel(0)
        .setLevelCount(level_count)
        .setBaseArrayLayer(0)
        .setLayerCount(layer_count);

    vk::ComponentMapping components;
    components.setR(vk::ComponentSwizzle::eR)
        .setG(vk::ComponentSwizzle::eG)
        .setB(vk::ComponentSwizzle::eB)
        .setA(vk::ComponentSwizzle::eA);

    create_info.setImage(image)
        .setFormat(format)
        .setViewType(view_type)
        .setComponents(components)
        .setSubresourceRange(subresource_range);

    return manager->device->device.createImageView(create_info);
}

std::vector<char> readFile(const std::string& filename) {
    std::vector<char> buffer(0);
    std::ifstream file(filename, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        WEN_ERROR("Open file \"{}\" failed!", filename)
        return buffer;
    }
    size_t size = static_cast<size_t>(file.tellg());
    file.seekg(0);
    buffer.resize(size);
    file.read(buffer.data(), size);
    file.close();
    return buffer;
}

} // namespace wen