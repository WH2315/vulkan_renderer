#include "resources/descriptor/cubemap_texture.hpp"
#include "resources/buffer.hpp"
#include <ktxvulkan.h>
#include "base/utils.hpp"
#include "manager.hpp"

namespace wen {

CubemapTexture::CubemapTexture(const std::string& filename) {
    ktxTexture* ktx_texture;
    auto result = ktxTexture_CreateFromNamedFile(
        filename.c_str(),
        KTX_TEXTURE_CREATE_LOAD_IMAGE_DATA_BIT,
        &ktx_texture
    );
    assert(result == KTX_SUCCESS);

    mip_levels_ = ktx_texture->numLevels;
    ktx_uint8_t* data = ktxTexture_GetData(ktx_texture);
    ktx_size_t size = ktxTexture_GetDataSize(ktx_texture);

    // Copy texture data into staging buffer
    Buffer staging_buffer(
        size,
        vk::BufferUsageFlagBits::eTransferSrc,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
            VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT
    );
    memcpy(staging_buffer.map(), data, size);
    staging_buffer.unmap();

    // Create optimal tiled target image
    image_ = std::make_unique<Image>(
        ktx_texture->baseWidth,
        ktx_texture->baseHeight,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferSrc | vk::ImageUsageFlagBits::eTransferDst,
        vk::SampleCountFlagBits::e1,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT,
        mip_levels_,
        6,
        vk::ImageCreateFlagBits::eCubeCompatible
    );

    auto cmdbuf = manager->command_pool->allocateSingleUse();

    // Setup buffer copy regions for each face including all of its miplevels
    std::vector<vk::BufferImageCopy> regions;
    uint32_t offset = 0;

    for (uint32_t face = 0; face < 6; face++) {
        for (uint32_t level = 0; level < mip_levels_; level++) {
            // Calculate offset into staging buffer for the current mip level and face
            ktx_size_t offset;
            KTX_error_code ret = ktxTexture_GetImageOffset(ktx_texture, level, 0, face, &offset);
            assert(ret == KTX_SUCCESS);
            vk::BufferImageCopy region = {};
            region.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
            region.imageSubresource.mipLevel = level;
            region.imageSubresource.baseArrayLayer = face;
            region.imageSubresource.layerCount = 1;
            region.imageExtent.width = ktx_texture->baseWidth >> level;
            region.imageExtent.height = ktx_texture->baseHeight >> level;
            region.imageExtent.depth = 1;
            region.bufferOffset = offset;
            regions.push_back(region);
        }
    }

    // Image barrier for optimal image (target)
    // Set initial layout for all array layers (faces) of the optimal (target) tiled texture
    vk::ImageSubresourceRange range = {};
    range.aspectMask = vk::ImageAspectFlagBits::eColor;
    range.baseMipLevel = 0;
    range.levelCount = mip_levels_;
    range.layerCount = 6;

    vk::ImageMemoryBarrier barrier = {};
    barrier.image = image_->image;
    barrier.srcQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.dstQueueFamilyIndex = vk::QueueFamilyIgnored;
    barrier.subresourceRange = range;
    barrier.oldLayout = vk::ImageLayout::eUndefined;
    barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eNone;
    barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

    cmdbuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eAllCommands,
        vk::PipelineStageFlagBits::eAllCommands,
        vk::DependencyFlagBits::eByRegion,
        nullptr,
        nullptr,
        barrier
    );

    // Copy the cube map faces from the staging buffer to the optimal tiled image
    cmdbuf.copyBufferToImage(
        staging_buffer.buffer,
        image_->image,
        vk::ImageLayout::eTransferDstOptimal,
        regions
    );

    // Change texture image layout to shader read after all faces have been copied
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    cmdbuf.pipelineBarrier(
        vk::PipelineStageFlagBits::eAllCommands,
        vk::PipelineStageFlagBits::eAllCommands,
        vk::DependencyFlagBits::eByRegion,
        nullptr,
        nullptr,
        barrier
    );

    manager->command_pool->freeSingleUse(cmdbuf);

    image_view_ = createImageView(
        image_->image,
        vk::Format::eR8G8B8A8Unorm,
        vk::ImageAspectFlagBits::eColor,
        mip_levels_,
        6,
        vk::ImageViewType::eCube
    );

    ktxTexture_Destroy(ktx_texture);
}

CubemapTexture::~CubemapTexture() {
    manager->device->device.destroyImageView(image_view_);
    image_.reset();
}

} // namespace wen