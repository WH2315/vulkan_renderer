#pragma once

#include "core/window.hpp"
#include <vulkan/vulkan.hpp>
#include <optional>

namespace wen {

struct Configuration {
    void resetSize(uint32_t width, uint32_t height);
    uint32_t getWidth() const { return window_info.width; }
    uint32_t getHeight() const { return window_info.height; }
    bool msaa() const { return msaa_samples != vk::SampleCountFlagBits::e1; }
    void setSampleCount(vk::SampleCountFlagBits samples);
    vk::SampleCountFlagBits getSampleCount() const { return msaa_samples; }

    WindowInfo window_info;
    bool debug = false;
    std::string app_name;
    std::string engine_name;

    std::optional<vk::SurfaceFormatKHR> desired_format = std::nullopt;
    std::optional<vk::PresentModeKHR> desired_mode = std::nullopt;
    bool vsync = false;

    uint32_t max_frames_in_flight = 2;

private:
    vk::SampleCountFlagBits msaa_samples = vk::SampleCountFlagBits::e1;
};

extern Configuration* renderer_config;

} // namespace wen