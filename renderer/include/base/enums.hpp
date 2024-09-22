#pragma once

#include <vulkan/vulkan.hpp>

namespace wen {

const std::string SWAPCHAIN_IMAGE_ATTACHMENT = "swapchain_image_attachment";
const std::string DEPTH_ATTACHMENT = "depth_attachment";
const std::string EXTERNAL_SUBPASS = "external_subpass";

enum class ShaderStage {
    eVertex,
    eFragment,
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