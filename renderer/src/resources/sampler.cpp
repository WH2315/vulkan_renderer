#include "resources/sampler.hpp"
#include "manager.hpp"

namespace wen {

Sampler::Sampler(const SamplerOptions& options) {
    vk::SamplerCreateInfo create_info;
    create_info.setMagFilter(options.mag_filter)
        .setMinFilter(options.min_filter)
        .setAddressModeU(options.address_mode_u)
        .setAddressModeV(options.address_mode_v)
        .setAddressModeW(options.address_mode_w)
        .setAnisotropyEnable(options.max_anisotropy > 1)
        .setMaxAnisotropy(options.max_anisotropy)
        .setBorderColor(options.border_color)
        .setUnnormalizedCoordinates(false)
        .setCompareOp(vk::CompareOp::eAlways)
        .setMipmapMode(options.mipmap_mode)
        .setMinLod(0.0f)
        .setMaxLod(static_cast<float>(options.mip_levels))
        .setMipLodBias(0.0f);
    sampler = manager->device->device.createSampler(create_info);
}

Sampler::~Sampler() {
    manager->device->device.destroySampler(sampler);
}

} // namespace wen