#pragma once

#include <vulkan/vulkan.hpp>
#include <vk_mem_alloc.h>

namespace wen {

class Image {
public:
    Image(uint32_t width, uint32_t height, vk::Format format,
          vk::ImageUsageFlags image_usage, vk::SampleCountFlagBits samples,
          VmaMemoryUsage usage, VmaAllocationCreateFlags flags,
          uint32_t mip_levels = 1);
    ~Image();

    vk::Image image;

private:
    VmaAllocation allocation_;
};

} // namespace wen