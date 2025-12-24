#pragma once

#include <vulkan/vulkan.hpp>

namespace wen {

class Swapchain final {
public:
    Swapchain();
    ~Swapchain();

public:
    vk::SwapchainKHR swapchain;
    vk::SurfaceFormatKHR format;
    vk::PresentModeKHR mode;
    uint32_t image_count;
    std::vector<vk::Image> images;
    std::vector<vk::ImageView> image_views;

private:
    struct SwapchainSupportDetails {
        vk::SurfaceCapabilitiesKHR capabilities;
        std::vector<vk::SurfaceFormatKHR> formats;
        std::vector<vk::PresentModeKHR> modes;
    };
};

} // namespace wen