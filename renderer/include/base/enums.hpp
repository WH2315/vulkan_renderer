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

enum class InputRate {
    eVertex,
    eInstance,
};

enum class VertexType {
    eInt32, eInt32x2, eInt32x3, eInt32x4,
    eUint32, eUint32x2, eUint32x3, eUint32x4,
    eFloat, eFloat2, eFloat3, eFloat4,
};

enum class IndexType {
    eUint16,
    eUint32,
};

} // namespace wen