#include "resources/descriptor/data_texture.hpp"
#include "resources/buffer.hpp"
#include "base/utils.hpp"
#include "manager.hpp"

namespace wen {

static void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height) {
    vk::BufferImageCopy region;
    region.setBufferOffset(0)
        .setBufferRowLength(0)
        .setBufferImageHeight(0)
        .setImageSubresource({vk::ImageAspectFlagBits::eColor, 0, 0, 1})
        .setImageOffset({0, 0, 0})
        .setImageExtent({width, height, 1});
    
    auto cmdbuf = manager->command_pool->allocateSingleUse();
    cmdbuf.copyBufferToImage(buffer, image, vk::ImageLayout::eTransferDstOptimal, region);
    manager->command_pool->freeSingleUse(cmdbuf);
}

static void generateMipmaps(vk::Image image, vk::Format format, uint32_t width, uint32_t height, uint32_t mip_levels) {
    vk::FormatProperties properties = manager->device->physical_device.getFormatProperties(format);
    if (!(properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eSampledImageFilterLinear)) {
        WEN_ERROR("Texture image format does not support linear blitting")
        return;
    }

    auto cmdbuf = manager->command_pool->allocateSingleUse();
    vk::ImageMemoryBarrier barrier;
    barrier.setImage(image)
        .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setSubresourceRange({
            vk::ImageAspectFlagBits::eColor,
            0, 1, 0, 1
        });

    int32_t mip_width = width, mip_height = height;
    for (uint32_t i = 1; i < mip_levels; i++) {
        barrier.subresourceRange.setBaseMipLevel(i - 1);
        barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
            .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setDstAccessMask(vk::AccessFlagBits::eTransferRead);
        cmdbuf.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eTransfer,
            vk::DependencyFlagBits::eByRegion,
            nullptr,
            nullptr,
            barrier
        );

        vk::ImageBlit blit;
        blit.setSrcOffsets({{{0, 0, 0}, {mip_width, mip_height, 1}}})
            .setSrcSubresource({
                vk::ImageAspectFlagBits::eColor,
                i - 1,
                0,
                1
            })
            .setDstOffsets({{{0, 0, 0}, {mip_width > 1 ? mip_width / 2 : 1, mip_height > 1 ? mip_height / 2 : 1, 1}}})
            .setDstSubresource({
                vk::ImageAspectFlagBits::eColor,
                i,
                0,
                1
            });
        cmdbuf.blitImage(
            image,
            vk::ImageLayout::eTransferSrcOptimal,
            image,
            vk::ImageLayout::eTransferDstOptimal,
            blit,
            vk::Filter::eLinear
        );

        barrier.setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setSrcAccessMask(vk::AccessFlagBits::eTransferRead)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead);
        cmdbuf.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eFragmentShader,
            vk::DependencyFlagBits::eByRegion,
            nullptr,
            nullptr,
            barrier
        );

        if (mip_width > 1) {
            mip_width /= 2;
        }
        if (mip_height > 1) {
            mip_height /= 2;
        }
    }

    barrier.subresourceRange.setBaseMipLevel(mip_levels - 1);
    barrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal)
        .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
        .setDstAccessMask(vk::AccessFlagBits::eShaderRead);
    cmdbuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eFragmentShader,
        vk::DependencyFlagBits::eByRegion,
        nullptr,
        nullptr,
        barrier
    );
    manager->command_pool->freeSingleUse(cmdbuf);
}

DataTexture::DataTexture(const uint8_t* data, uint32_t width, uint32_t height, uint32_t mip_levels) {
    uint32_t size = width * height * 4;
    Buffer staging_buffer(
        size,
        vk::BufferUsageFlagBits::eTransferSrc,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT
    );
    memcpy(staging_buffer.map(), data, size);
    staging_buffer.unmap();

    if (mip_levels == 0) {
        mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height))) + 1);
    }
    mip_levels_ = mip_levels;

    image_ = std::make_unique<Image>(
        width, height,
        vk::Format::eR8G8B8A8Srgb,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst,
        vk::SampleCountFlagBits::e1,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        mip_levels
    );

    transitionImageLayout(
        image_->image,
        vk::ImageAspectFlagBits::eColor,
        mip_levels,
        {
            vk::ImageLayout::eUndefined,
            vk::AccessFlagBits::eNone,
            vk::PipelineStageFlagBits::eTopOfPipe,
        },
        {
            vk::ImageLayout::eTransferDstOptimal,
            vk::AccessFlagBits::eTransferWrite,
            vk::PipelineStageFlagBits::eTransfer,
        }
    );

    copyBufferToImage(staging_buffer.buffer, image_->image, width, height);
    generateMipmaps(image_->image, vk::Format::eR8G8B8A8Srgb, width, height, mip_levels);

    image_view_ = createImageView(image_->image, vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor, mip_levels);
}

DataTexture::~DataTexture() {
    manager->device->device.destroyImageView(image_view_);
    image_.reset();
}

} // namespace wen