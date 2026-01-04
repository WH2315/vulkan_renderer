#pragma once

#include "base/enums.hpp"
#include "resources/descriptor/uniform_buffer.hpp"
#include "resources/specific_texture.hpp"
#include "resources/sampler.hpp"

namespace wen {

class Renderer;
class RenderPipeline;

struct DescriptorSetLayoutBinding {
    DescriptorSetLayoutBinding(uint32_t binding, vk::DescriptorType descriptor_type, ShaderStages stage)
        : binding(binding), descriptor_type(descriptor_type), descriptor_count(1), stage(stage), samples(nullptr) {}
    
    DescriptorSetLayoutBinding(uint32_t binding, vk::DescriptorType descriptor_type, uint32_t count, ShaderStages stage)
        : binding(binding), descriptor_type(descriptor_type), descriptor_count(count), stage(stage), samples(nullptr) {}

    uint32_t binding;
    vk::DescriptorType descriptor_type;
    uint32_t descriptor_count;
    ShaderStages stage;
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
    void bindTextures(uint32_t binding, const std::vector<std::pair<std::shared_ptr<SpecificTexture>, std::shared_ptr<Sampler>>>& textures_samplers);
    void bindTexture(uint32_t binding, std::shared_ptr<SpecificTexture> texture, std::shared_ptr<Sampler> sampler);
    void bindInputAttachments(uint32_t binding, const std::shared_ptr<Renderer>& renderer, const std::vector<std::pair<std::string, std::shared_ptr<Sampler>>>& names_samplers);
    void bindInputAttachment(uint32_t binding, const std::shared_ptr<Renderer>& renderer, const std::string& name, std::shared_ptr<Sampler> sampler);

private:
    std::vector<vk::DescriptorSetLayoutBinding> bindings_;
    vk::DescriptorSetLayout descriptor_layout_;
    std::vector<vk::DescriptorSet> descriptor_sets_;
};

} // namespace wen