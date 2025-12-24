#pragma once

#include "core/log.hpp"
#include <vulkan/vulkan.hpp>

#define VK_CALL(expr)                                                    \
    do {                                                                 \
        auto result = expr;                                              \
        WEN_DEBUG("Vulkan call: {} at {}", vk::to_string(result), #expr) \
    } while (0)