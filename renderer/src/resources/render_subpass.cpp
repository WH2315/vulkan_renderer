#include "resources/render_subpass.hpp"
#include "resources/render_pass.hpp"
#include "base/configuration.hpp"
#include "core/log.hpp"

namespace wen {

RenderSubpass::RenderSubpass(const std::string& name, RenderPass& render_pass)
    : name(name), render_pass_(render_pass) {}

RenderSubpass::~RenderSubpass() {}

void RenderSubpass::setOutputAttachment(const std::string& name, vk::ImageLayout layout) {
    output_attachments_.push_back(createAttachmentReference(name, layout, false, "setOutputAttachment"));
    color_blend_attachments.push_back({
        false,
        vk::BlendFactor::eZero,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::BlendFactor::eZero,
        vk::BlendFactor::eZero,
        vk::BlendOp::eAdd,
        vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
    });
    if (renderer_config->msaa()) {
        resolve_attachments_.push_back(createAttachmentReference(name, layout, true, "setOutputAttachment(resolve)"));
    }
}

void RenderSubpass::setDepthAttachment(const std::string& name, vk::ImageLayout layout) {
    depth_attachment_ = createAttachmentReference(name, layout, false, "setDepthAttachment");
}

void RenderSubpass::setInputAttachment(const std::string& name, vk::ImageLayout layout) {
    input_attachments_.push_back(createAttachmentReference(name, layout, true, "setInputAttachment"));
}

vk::SubpassDescription RenderSubpass::build() {
    vk::SubpassDescription subpass = {};
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        .setColorAttachmentCount(output_attachments_.size())
        .setColorAttachments(output_attachments_);
    if (depth_attachment_.has_value()) {
        subpass.setPDepthStencilAttachment(&depth_attachment_.value());
    } 
    if (renderer_config->msaa()) {
        subpass.setResolveAttachments(resolve_attachments_);
    }
    subpass.setInputAttachmentCount(input_attachments_.size())
        .setInputAttachments(input_attachments_);
    return subpass;
}

vk::AttachmentReference RenderSubpass::createAttachmentReference(const std::string& name, vk::ImageLayout layout, bool read, const char* caller) {
    uint32_t attachment = render_pass_.getAttachmentIndex(name, read);
    std::string label =
        (!renderer_config->msaa() || !read) ? "attachment" : "resolve_attachment";
    WEN_DEBUG("\"{}\": {} -> {}_index: {}, name: {}", this->name,
              caller ? caller : "unknown", label, attachment, name)
    vk::AttachmentReference reference = {};
    reference.setAttachment(attachment).setLayout(layout);
    return reference;
}

} // namespace wen