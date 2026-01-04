#include "resources/descriptor/descriptor_set.hpp"
#include "renderer.hpp"
#include "base/utils.hpp"
#include "manager.hpp"

namespace wen {

DescriptorSet::~DescriptorSet() {
    bindings_.clear();
    manager->device->device.destroyDescriptorSetLayout(descriptor_layout_);
    manager->descriptor_pool->free(descriptor_sets_);
    descriptor_sets_.clear();
}

DescriptorSet& DescriptorSet::addDescriptors(const std::vector<DescriptorSetLayoutBinding>& bindings) {
    for (auto binding : bindings) {
        bindings_.push_back({
            binding.binding,
            binding.descriptor_type,
            binding.descriptor_count,
            convert<vk::ShaderStageFlags>(binding.stage),
            binding.samples
        });
    }
    return *this;
}

void DescriptorSet::build() {
    vk::DescriptorSetLayoutCreateInfo create_info;
    create_info.setBindings(bindings_);
    descriptor_layout_ = manager->device->device.createDescriptorSetLayout(create_info);
    descriptor_sets_ = manager->descriptor_pool->allocate(descriptor_layout_);
}

const vk::DescriptorSetLayoutBinding& DescriptorSet::getBinding(uint32_t binding) {
    for (const auto& b : bindings_) {
        if (b.binding == binding) {
            return b;
        }
    }
    WEN_ERROR("binding {} not found!", binding)
    return *bindings_.end();
}

void DescriptorSet::bindUniforms(uint32_t binding, const std::vector<std::shared_ptr<UniformBuffer>>& uniform_buffers) {
    auto layout_binding = getBinding(binding);
    if (layout_binding.descriptorType != vk::DescriptorType::eUniformBuffer) {
        WEN_ERROR("binding {} is not uniform buffer!", binding)
        return;
    }
    if (layout_binding.descriptorCount != uniform_buffers.size()) {
        WEN_ERROR("binding {} requires {} uniform buffers, but {} provided!", binding, layout_binding.descriptorCount, uniform_buffers.size())
        return;
    }
    for (uint32_t i = 0; i < renderer_config->max_frames_in_flight; i++) {
        std::vector<vk::DescriptorBufferInfo> buffers(layout_binding.descriptorCount);
        for (uint32_t j = 0; j < layout_binding.descriptorCount; j++) {
            buffers[j].setBuffer(uniform_buffers[j]->getBuffer())
                .setOffset(0)
                .setRange(uniform_buffers[j]->getSize());
        }
        vk::WriteDescriptorSet write;
        write.setDstSet(descriptor_sets_[i])
            .setDstBinding(layout_binding.binding)
            .setDstArrayElement(0)
            .setDescriptorType(layout_binding.descriptorType)
            .setBufferInfo(buffers);
        manager->device->device.updateDescriptorSets({write}, {});
    }
}

void DescriptorSet::bindUniform(uint32_t binding, std::shared_ptr<UniformBuffer> uniform_buffer) {
    bindUniforms(binding, {uniform_buffer});
}

void DescriptorSet::bindTextures(uint32_t binding, const std::vector<std::pair<std::shared_ptr<SpecificTexture>, std::shared_ptr<Sampler>>>& textures_samplers) {
    auto layout_binding = getBinding(binding);
    if (layout_binding.descriptorType != vk::DescriptorType::eCombinedImageSampler) {
        WEN_ERROR("binding {} is not combined image sampler!", binding)
        return;
    }
    if (layout_binding.descriptorCount != textures_samplers.size()) {
        WEN_ERROR("binding {} requires {} textures, but {} provided!", binding, layout_binding.descriptorCount, textures_samplers.size())
        return;
    }
    for (uint32_t i = 0; i < renderer_config->max_frames_in_flight; i++) {
        std::vector<vk::DescriptorImageInfo> images(layout_binding.descriptorCount);
        for (uint32_t j = 0; j < layout_binding.descriptorCount; j++) {
            images[j].setImageLayout(textures_samplers[j].first->getImageLayout())
                .setImageView(textures_samplers[j].first->getImageView())
                .setSampler(textures_samplers[j].second->sampler);
        }
        vk::WriteDescriptorSet write;
        write.setDstSet(descriptor_sets_[i])
            .setDstBinding(layout_binding.binding)
            .setDstArrayElement(0)
            .setDescriptorType(layout_binding.descriptorType)
            .setImageInfo(images);
        manager->device->device.updateDescriptorSets({write}, {});
    }
}

void DescriptorSet::bindTexture(uint32_t binding, std::shared_ptr<SpecificTexture> texture, std::shared_ptr<Sampler> sampler) {
    bindTextures(binding, {{texture, sampler}});
}

void DescriptorSet::bindInputAttachments(uint32_t binding, const std::shared_ptr<Renderer>& renderer, const std::vector<std::pair<std::string, std::shared_ptr<Sampler>>>& names_samplers) {
    auto layout_binding = getBinding(binding);
    if ((layout_binding.descriptorType != vk::DescriptorType::eInputAttachment) &&
        (layout_binding.descriptorType != vk::DescriptorType::eCombinedImageSampler)) {
        WEN_ERROR("binding {} is not input attachment!", binding)
        return;
    }
    if (layout_binding.descriptorCount != names_samplers.size()) {
        WEN_ERROR("binding {} requires {} input attachments, but {} provided!", binding, layout_binding.descriptorCount, names_samplers.size())
        return;
    }
    for (uint32_t i = 0; i < renderer_config->max_frames_in_flight; i++) {
        std::vector<vk::DescriptorImageInfo> images(layout_binding.descriptorCount);
        for (uint32_t j = 0; j < layout_binding.descriptorCount; j++) {
            auto index = renderer->render_pass->getAttachmentIndex(names_samplers[j].first, true);
            images[j].setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .setImageView(renderer->framebuffer_set->attachments[index]->image_view)
                .setSampler(names_samplers[j].second->sampler);
        }
        vk::WriteDescriptorSet write;
        write.setDstSet(descriptor_sets_[i])
            .setDstBinding(layout_binding.binding)
            .setDstArrayElement(0)
            .setDescriptorType(layout_binding.descriptorType)
            .setImageInfo(images);
        manager->device->device.updateDescriptorSets({write}, {});
    }
}

void DescriptorSet::bindInputAttachment(uint32_t binding, const std::shared_ptr<Renderer>& renderer, const std::string& name, std::shared_ptr<Sampler> sampler) {
    bindInputAttachments(binding, renderer, {{name, sampler}});
}

} // namespace wen