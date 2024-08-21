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

std::string readFile(const std::string& filename) {
    std::string result;
    std::ifstream file(filename, std::ios::in | std::ios::binary);
    if (file) {
        file.seekg(0, std::ios::end);
        size_t size = file.tellg();
        if (size != -1) {
            result.resize(size);
            file.seekg(0, std::ios::beg);
            file.read(&result[0], size);
            file.close();
        } else {
            WEN_ERROR("Could not read file '{0}'", filename)
        }
    } else {
        WEN_ERROR("Could not open file '{0}'", filename)
    }
    return result;
}

} // namespace wen