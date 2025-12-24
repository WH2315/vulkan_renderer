#include "basic/descriptor_pool.hpp"
#include "manager.hpp"

namespace wen {

DescriptorPool::DescriptorPool() {
    std::vector<DescriptorPool::PoolSizeRatio> pool_ratios = {
        {vk::DescriptorType::eUniformBuffer, 1.0f},
        {vk::DescriptorType::eCombinedImageSampler, 1.0f}
    };
    init(10, pool_ratios);
}

DescriptorPool::~DescriptorPool() {
    clearPools();
    for (auto& pool : ready_pools_) {
        manager->device->device.destroyDescriptorPool(pool);
	}
    ready_pools_.clear();
	for (auto& pool : full_pools_) {
        manager->device->device.destroyDescriptorPool(pool);
    }
    full_pools_.clear();
}

void DescriptorPool::init(uint32_t max_sets, std::span<PoolSizeRatio> pool_ratios) {
    ratios_.clear();    
    for (auto ratio : pool_ratios) {
        ratios_.push_back(ratio);
    }
    auto new_pool = createPool(max_sets, pool_ratios);
    sets_per_pool_ = max_sets * 1.5;
    ready_pools_.push_back(new_pool);
}

void DescriptorPool::clearPools() {
    for (auto pool : ready_pools_) {
        manager->device->device.resetDescriptorPool(pool);
    }
    for (auto pool : full_pools_) {
        manager->device->device.resetDescriptorPool(pool);
        ready_pools_.push_back(pool);
    }
    full_pools_.clear();
}

std::vector<vk::DescriptorSet> DescriptorPool::allocate(vk::DescriptorSetLayout layout) {
    auto pool_to_use = getPool();
    std::vector<vk::DescriptorSet> descriptor_sets(renderer_config->max_frames_in_flight);
    std::vector<vk::DescriptorSetLayout> layouts(renderer_config->max_frames_in_flight, layout);
    vk::DescriptorSetAllocateInfo allocate_info;
    allocate_info.setDescriptorPool(pool_to_use)
        .setDescriptorSetCount(static_cast<uint32_t>(renderer_config->max_frames_in_flight))
        .setSetLayouts(layouts);
    try {
        descriptor_sets = manager->device->device.allocateDescriptorSets(allocate_info);
    } catch (vk::OutOfPoolMemoryError) {
        full_pools_.push_back(pool_to_use);
        pool_to_use = getPool();
        allocate_info.setDescriptorPool(pool_to_use);
        descriptor_sets = manager->device->device.allocateDescriptorSets(allocate_info);
    }
    ready_pools_.push_back(pool_to_use);
    return descriptor_sets;
}

void DescriptorPool::free(const std::vector<vk::DescriptorSet>& descriptor_sets) {
    auto pool_to_use = getPool();
    manager->device->device.freeDescriptorSets(pool_to_use, descriptor_sets);
    ready_pools_.push_back(pool_to_use);
}

vk::DescriptorPool DescriptorPool::createPool(uint32_t set_count, std::span<PoolSizeRatio> pool_ratios) {
    std::vector<vk::DescriptorPoolSize> pool_size;
    for (auto ratio : pool_ratios) {
        pool_size.push_back({
            ratio.descriptor_type,
            static_cast<uint32_t>(set_count * ratio.ratio)
        });
    }
    vk::DescriptorPoolCreateInfo create_info;
    create_info.setPoolSizeCount(static_cast<uint32_t>(pool_size.size()))
        .setPoolSizes(pool_size)
        .setMaxSets(set_count)
        .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
    return manager->device->device.createDescriptorPool(create_info);
}

vk::DescriptorPool DescriptorPool::getPool() {
    vk::DescriptorPool new_pool;
    if (!ready_pools_.empty()) {
        new_pool = ready_pools_.back();
        ready_pools_.pop_back();
    } else {
        new_pool = createPool(sets_per_pool_, ratios_);
        sets_per_pool_ *= 1.5;
        if (sets_per_pool_ > 4092) {
            sets_per_pool_ = 4092;
        }
    }
    return new_pool;
}

} // namespace wen