#include "base/configuration.hpp"
#include "base/utils.hpp"
#include "core/log.hpp"

namespace wen {

Configuration* renderer_config = nullptr;

void Configuration::resetSize(uint32_t width, uint32_t height) {
    window_info.width = width;
    window_info.height = height;
}

void Configuration::setSampleCount(vk::SampleCountFlagBits samples) {
    static auto max_samples = getMaxUsableSampleCount();
    if (static_cast<uint32_t>(samples) > static_cast<uint32_t>(max_samples)) {
        WEN_WARN("max usable sample count is {}", static_cast<uint32_t>(max_samples))
        samples = max_samples;
    }
    msaa_samples = samples;
}

} // namespace wen