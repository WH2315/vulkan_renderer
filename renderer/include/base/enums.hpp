#pragma once

#include <vulkan/vulkan.hpp>

namespace wen {

enum class ShaderStage {
    eVertex = static_cast<uint32_t>(vk::ShaderStageFlagBits::eVertex),
    eFragment = static_cast<uint32_t>(vk::ShaderStageFlagBits::eFragment),
};

enum class AttachmentType {
    eColor,
    eDepth,
};

} // namespace wen