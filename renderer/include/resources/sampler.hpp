#pragma once

#include <vulkan/vulkan.hpp>

namespace wen {

struct SamplerOptions {
    vk::Filter mag_filter = vk::Filter::eLinear;
    vk::Filter min_filter = vk::Filter::eLinear;
    vk::SamplerAddressMode address_mode_u = vk::SamplerAddressMode::eRepeat;
    vk::SamplerAddressMode address_mode_v = vk::SamplerAddressMode::eRepeat;
    vk::SamplerAddressMode address_mode_w = vk::SamplerAddressMode::eRepeat;
    uint32_t max_anisotropy = 1;
    vk::BorderColor border_color = vk::BorderColor::eIntOpaqueBlack;
    vk::SamplerMipmapMode mipmap_mode = vk::SamplerMipmapMode::eLinear;
    uint32_t mip_levels = 1;
};

class Sampler {
public:
    Sampler(const SamplerOptions& options);
    ~Sampler();

    vk::Sampler sampler;
};

} // namespace wen