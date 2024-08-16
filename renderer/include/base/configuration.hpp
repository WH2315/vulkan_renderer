#pragma once

#include "core/window.hpp"

namespace wen {

struct Configuration {
    void resetSize(uint32_t width, uint32_t height);
    uint32_t getWidth() const { return window_info.width; }
    uint32_t getHeight() const { return window_info.height; }

    WindowInfo window_info;
    bool debug = false;
    std::string app_name;
    std::string engine_name;
};

extern Configuration* renderer_config;

} // namespace wen