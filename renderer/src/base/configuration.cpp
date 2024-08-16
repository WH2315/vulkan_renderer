#include "base/configuration.hpp"

namespace wen {

Configuration* renderer_config = nullptr;

void Configuration::resetSize(uint32_t width, uint32_t height) {
    window_info.width = width;
    window_info.height = height;
}

} // namespace wen