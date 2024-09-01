#pragma once

#include <vulkan/vulkan.hpp>
#include <optional>

namespace wen {

class RenderPass;
class RenderSubpass {
public:
    RenderSubpass(const std::string& name, RenderPass& render_pass);
    ~RenderSubpass();

    void setOutputAttachment(const std::string& name, vk::ImageLayout layout = vk::ImageLayout::eColorAttachmentOptimal);
    void setDepthAttachment(const std::string& name, vk::ImageLayout layout = vk::ImageLayout::eDepthStencilAttachmentOptimal);

    vk::SubpassDescription build();

public:
    std::string name;
    std::vector<vk::PipelineColorBlendAttachmentState> color_blend_attachments;

private:
    RenderPass& render_pass_;
    std::vector<vk::AttachmentReference> output_attachments_;
    std::optional<vk::AttachmentReference> depth_attachment_;

private:
    vk::AttachmentReference createAttachmentReference(const std::string& name, vk::ImageLayout layout);
};

} // namespace wen