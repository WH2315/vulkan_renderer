#pragma once

#include "core/window.hpp"
#include <vulkan/vulkan.hpp>
#include <optional>

namespace wen {

struct Configuration {
    void resetSize(uint32_t width, uint32_t height);
    uint32_t getWidth() const { return window_info.width; }
    uint32_t getHeight() const { return window_info.height; }

    WindowInfo window_info;
    bool debug = false;
    std::string app_name;
    std::string engine_name;

    std::optional<vk::SurfaceFormatKHR> desired_format = std::nullopt;
    std::optional<vk::PresentModeKHR> desired_mode = std::nullopt;
    bool sync = false;

    uint32_t max_frames_in_flight = 2;
};

extern Configuration* renderer_config;

} // namespace wen