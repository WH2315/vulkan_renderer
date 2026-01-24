#include "resources/descriptor/ktx_texture.hpp"
#include "base/utils.hpp"
#include "manager.hpp"

namespace wen {

KtxTexture::KtxTexture(const std::string& filename) {
    auto result = ktxTexture_CreateFromNamedFile(
        filename.c_str(),
        KTX_TEXTURE_CREATE_NO_FLAGS,
        &ktx_texture_
    );
    assert(result == KTX_SUCCESS);

    ktxVulkanDeviceInfo kvdi;
    ktxVulkanDeviceInfo_Construct(&kvdi, manager->device->physical_device, manager->device->device, manager->device->transfer_queue, manager->command_pool->command_pool_, nullptr);
    result = ktxTexture_VkUploadEx(
        ktx_texture_,
        &kvdi,
        &vk_texture_,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    );
    assert(result == KTX_SUCCESS);

    image_view_ = createImageView(
        vk_texture_.image,
        vk::Format(vk_texture_.imageFormat),
        vk::ImageAspectFlagBits::eColor,
        vk_texture_.levelCount,
        vk_texture_.layerCount,
        vk::ImageViewType(vk_texture_.viewType)
    );
    ktxVulkanDeviceInfo_Destruct(&kvdi);
}

KtxTexture::~KtxTexture() {
    vk_texture_.vkFreeMemory(manager->device->device, vk_texture_.deviceMemory, nullptr);
    vk_texture_.vkDestroyImage(manager->device->device, vk_texture_.image, nullptr);
    ktxTexture_Destroy(ktx_texture_);
    manager->device->device.destroyImageView(image_view_);
}

} // namespace wen