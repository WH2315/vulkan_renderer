#pragma once

#include <vulkan/vulkan.hpp>
#include <span>

namespace wen {

class DescriptorPool {
public:
    struct PoolSizeRatio {
        vk::DescriptorType descriptor_type;
        float ratio;
    };

    DescriptorPool();
    ~DescriptorPool();

    void init(uint32_t max_sets, std::span<PoolSizeRatio> pool_ratios);
    void clearPools();

    std::vector<vk::DescriptorSet> allocate(vk::DescriptorSetLayout layout);
    void free(const std::vector<vk::DescriptorSet>& descriptor_sets);

private:
    vk::DescriptorPool createPool(uint32_t set_count, std::span<PoolSizeRatio> pool_ratios);
    vk::DescriptorPool getPool();

private:
    std::vector<PoolSizeRatio> ratios_;
    std::vector<vk::DescriptorPool> full_pools_;
    std::vector<vk::DescriptorPool> ready_pools_;
    uint32_t sets_per_pool_;
};

} // namespace wen