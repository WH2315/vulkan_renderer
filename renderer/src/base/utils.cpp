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

void transitionImageLayout(vk::Image image, vk::ImageAspectFlagBits aspect,
                           uint32_t mip_levels, const TransitionInfo& src,
                           const TransitionInfo& dst) {
    vk::ImageMemoryBarrier barrier;
    barrier.setOldLayout(src.layout)
        .setSrcAccessMask(src.access)
        .setNewLayout(dst.layout)
        .setDstAccessMask(dst.access)
        .setImage(image)
        .setSubresourceRange({
            aspect,
            0,
            mip_levels,
            0,
            1
        })
        .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setDstQueueFamilyIndex(vk::QueueFamilyIgnored);
    
    auto cmdbuf = manager->command_pool->allocateSingleUse();
    cmdbuf.pipelineBarrier(
        src.stage,
        dst.stage,
        vk::DependencyFlagBits::eByRegion,
        nullptr,
        nullptr,
        barrier
    );
    manager->command_pool->freeSingleUse(cmdbuf);
}

vk::Format findSupportedFormat(const std::vector<vk::Format>& candidates,
                               vk::ImageTiling tiling,
                               vk::FormatFeatureFlags features) {
    for (auto format : candidates) {
        auto props = manager->device->physical_device.getFormatProperties(format);
        if (tiling == vk::ImageTiling::eLinear && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == vk::ImageTiling::eOptimal && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    return vk::Format::eUndefined;
}

vk::Format findDepthFormat() {
    return findSupportedFormat(
        {vk::Format::eD32Sfloat, vk::Format::eD32SfloatS8Uint, vk::Format::eD24UnormS8Uint},
        vk::ImageTiling::eOptimal,
        vk::FormatFeatureFlagBits::eDepthStencilAttachment
    );
}

vk::SampleCountFlagBits getMaxUsableSampleCount() {
    auto properties = manager->device->physical_device.getProperties();
    vk::SampleCountFlags counts = properties.limits.framebufferColorSampleCounts & properties.limits.framebufferDepthSampleCounts;
    if (counts & vk::SampleCountFlagBits::e64) { return vk::SampleCountFlagBits::e64; }
    if (counts & vk::SampleCountFlagBits::e32) { return vk::SampleCountFlagBits::e32; }
    if (counts & vk::SampleCountFlagBits::e16) { return vk::SampleCountFlagBits::e16; }
    if (counts & vk::SampleCountFlagBits::e8) { return vk::SampleCountFlagBits::e8; }
    if (counts & vk::SampleCountFlagBits::e4) { return vk::SampleCountFlagBits::e4; }
    if (counts & vk::SampleCountFlagBits::e2) { return vk::SampleCountFlagBits::e2; }
    return vk::SampleCountFlagBits::e1;
}

} // namespace wen