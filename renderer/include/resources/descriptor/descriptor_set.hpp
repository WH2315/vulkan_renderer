#pragma once

#include "base/enums.hpp"
#include "resources/descriptor/uniform_buffer.hpp"

namespace wen {

struct DescriptorSetLayoutBinding {
    DescriptorSetLayoutBinding(uint32_t binding, vk::DescriptorType descriptor_type, ShaderStage stage)
        : binding(binding), descriptor_type(descriptor_type), descriptor_count(1), stage(stage), samples(nullptr) {}

    uint32_t binding;
    vk::DescriptorType descriptor_type;
    uint32_t descriptor_count;
    ShaderStage stage;
    const vk::Sampler* samples;
};

class DescriptorSet {
    friend class RenderPipeline; // descriptor_layout_
    friend class Renderer; // descriptor_sets_

public:
    DescriptorSet() = default;
    ~DescriptorSet();

    DescriptorSet& addDescriptors(const std::vector<DescriptorSetLayoutBinding>& bindings);
    void build();

    const vk::DescriptorSetLayoutBinding& getBinding(uint32_t binding);

    void bindUniforms(uint32_t binding, const std::vector<std::shared_ptr<UniformBuffer>>& uniform_buffers);
    void bindUniform(uint32_t binding, std::shared_ptr<UniformBuffer> uniform_buffer);

private:
    std::vector<vk::DescriptorSetLayoutBinding> bindings_;
    vk::DescriptorSetLayout descriptor_layout_;
    std::vector<vk::DescriptorSet> descriptor_sets_;
};

} // namespace wen