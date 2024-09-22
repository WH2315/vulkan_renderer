#include "resources/render_subpass.hpp"
#include "resources/render_pass.hpp"
#include "base/configuration.hpp"

namespace wen {

RenderSubpass::RenderSubpass(const std::string& name, RenderPass& render_pass)
    : name(name), render_pass_(render_pass) {}

RenderSubpass::~RenderSubpass() {}

void RenderSubpass::setOutputAttachment(const std::string& name, vk::ImageLayout layout) {
    output_attachments_.push_back(createAttachmentReference(name, layout, false));
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
        resolve_attachments_.push_back(createAttachmentReference(name, layout, true));
    }
}

void RenderSubpass::setDepthAttachment(const std::string& name, vk::ImageLayout layout) {
    depth_attachment_ = createAttachmentReference(name, layout, false);
}

vk::SubpassDescription RenderSubpass::build() {
    vk::SubpassDescription subpass = {};
    subpass.setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        .setColorAttachmentCount(output_attachments_.size())
        .setColorAttachments(output_attachments_);
    if (depth_attachment_.has_value()) {
        subpass.setPDepthStencilAttachment(&depth_attachment_.value());
    } 
    if (!resolve_attachments_.empty()) {
        subpass.setResolveAttachments(resolve_attachments_);
    }
    return subpass;
}

vk::AttachmentReference RenderSubpass::createAttachmentReference(const std::string& name, vk::ImageLayout layout, bool read) {
    uint32_t attachment = render_pass_.getAttachmentIndex(name, read);
    vk::AttachmentReference reference = {};
    reference.setAttachment(attachment)
        .setLayout(layout);
    return reference;
}

} // namespace wen