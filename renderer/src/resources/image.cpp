#include "resources/image.hpp"
#include "manager.hpp"

namespace wen {

Image::Image(uint32_t width, uint32_t height, vk::Format format,
             vk::ImageUsageFlags image_usage, vk::SampleCountFlagBits samples,
             VmaMemoryUsage usage, VmaAllocationCreateFlags flags,
             uint32_t mip_levels) {
    VmaAllocationCreateInfo allocation_create_info = {};
    allocation_create_info.usage = usage;
    allocation_create_info.flags = flags;

    vk::ImageCreateInfo image_create_info = {};
    image_create_info.setImageType(vk::ImageType::e2D)
        .setExtent({width, height, 1})
        .setMipLevels(mip_levels)
        .setArrayLayers(1)
        .setFormat(format)
        .setTiling(vk::ImageTiling::eOptimal)
        .setUsage(image_usage)
        .setSamples(samples)
        .setInitialLayout(vk::ImageLayout::eUndefined);
    
    vmaCreateImage(
        manager->vma_allocator,
        reinterpret_cast<VkImageCreateInfo*>(&image_create_info),
        &allocation_create_info,
        reinterpret_cast<VkImage*>(&image),
        &allocation_,
        nullptr
    );
}

Image::~Image() {
    vmaDestroyImage(manager->vma_allocator, image, allocation_);
}

} // namespace wen