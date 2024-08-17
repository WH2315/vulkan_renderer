#include "basic/swapchain.hpp"
#include "base/utils.hpp"
#include "manager.hpp"

namespace wen {

Swapchain::Swapchain() {
    SwapchainSupportDetails details = {
        .capabilities = manager->device->physical_device.getSurfaceCapabilitiesKHR(manager->surface),
        .formats = manager->device->physical_device.getSurfaceFormatsKHR(manager->surface),
        .modes = manager->device->physical_device.getSurfacePresentModesKHR(manager->surface)
    };

    format = details.formats[0];
    mode = details.modes[0];

    vk::SurfaceFormatKHR desired_format;
    if (renderer_config->desired_format.has_value()) {
        desired_format = renderer_config->desired_format.value();
    } else {
        desired_format = {vk::Format::eB8G8R8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear};
    }
    for (auto format : details.formats) {
        if (format.format == desired_format.format && format.colorSpace == desired_format.colorSpace) {
            this->format = format;
            break;
        }
    }

    vk::PresentModeKHR desired_mode;
    if (renderer_config->desired_mode.has_value()) {
        desired_mode = renderer_config->desired_mode.value();
    } else {
        desired_mode = vk::PresentModeKHR::eMailbox;
    }
    for (auto mode : details.modes) {
        if (mode == desired_mode) {
            this->mode = mode;
            break;
        }
    }

    image_count = std::clamp<uint32_t>(mode == vk::PresentModeKHR::eMailbox ? 4 : 3,
                                       details.capabilities.minImageCount,
                                       details.capabilities.maxImageCount);

    uint32_t width = std::clamp<uint32_t>(details.capabilities.currentExtent.width,
                                          details.capabilities.minImageExtent.width,
                                          details.capabilities.maxImageExtent.width);
    uint32_t height = std::clamp<uint32_t>(details.capabilities.currentExtent.height,
                                           details.capabilities.minImageExtent.height,
                                           details.capabilities.maxImageExtent.height);
    renderer_config->resetSize(width, height);

    vk::SwapchainCreateInfoKHR create_info;
    create_info.setOldSwapchain(nullptr)
        .setSurface(manager->surface)
        .setPreTransform(details.capabilities.currentTransform)
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
        .setPresentMode(mode)
        .setClipped(true)
        .setMinImageCount(image_count)
        .setImageFormat(format.format)
        .setImageColorSpace(format.colorSpace)
        .setImageExtent({width, height})
        .setImageArrayLayers(1)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment);

    uint32_t queue_family_indices[] = {
        manager->device->graphics_queue_family,
        manager->device->present_queue_family
    };
    if (queue_family_indices[0] != queue_family_indices[1]) {
        create_info.setImageSharingMode(vk::SharingMode::eConcurrent)
            .setQueueFamilyIndices(queue_family_indices);
    } else {
        create_info.setImageSharingMode(vk::SharingMode::eExclusive);
    }

    swapchain = manager->device->device.createSwapchainKHR(create_info);
    images = manager->device->device.getSwapchainImagesKHR(swapchain);
    image_views.resize(image_count);
    for (uint32_t i = 0; i < image_count; i++) {
        image_views[i] = createImageView(images[i], format.format, vk::ImageAspectFlagBits::eColor, 1);
    }
}

Swapchain::~Swapchain() {
    for (auto& image_view : image_views) {
        manager->device->device.destroyImageView(image_view);
    }
    image_views.clear();
    images.clear();
    manager->device->device.destroySwapchainKHR(swapchain);
}

} // namespace wen