#include "resources/render_pass.hpp"
#include "base/utils.hpp"
#include "manager.hpp"

namespace wen {

RenderPass::RenderPass() {}

RenderPass::~RenderPass() {
    attachments.clear();
    for (auto& subpass : subpasses) {
        subpass.reset();
    }
    subpasses.clear();

    manager->device->device.destroyRenderPass(render_pass);
    final_attachments.clear();
    final_subpasses.clear();
    final_dependencies.clear();
}

void RenderPass::addAttachment(const std::string& name, AttachmentType type) {
    attachment_indices_.insert(std::make_pair(name, attachments.size()));
    auto& attachment = attachments.emplace_back();

    attachment.write_attachment
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setInitialLayout(vk::ImageLayout::eUndefined); 
    
    switch (type) {
        case AttachmentType::eColor:
            attachment.write_attachment
                .setFormat(manager->swapchain->format.format)
                .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);
            attachment.write_usage = vk::ImageUsageFlagBits::eColorAttachment;
            attachment.aspect = vk::ImageAspectFlagBits::eColor;
            attachment.clear_color = vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
            break;
        case AttachmentType::eDepth:
            attachment.write_attachment
                .setFormat(findDepthFormat())
                .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
                .setStencilLoadOp(vk::AttachmentLoadOp::eClear);
            attachment.write_usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
            attachment.aspect = vk::ImageAspectFlagBits::eDepth;
            attachment.clear_color = {{1.0f, 0}};
            break;
    }

    static bool first = true;
    if (first) {
        attachment.write_attachment
            .setFinalLayout(vk::ImageLayout::ePresentSrcKHR);
        first = false;
    }
}

RenderSubpass& RenderPass::addSubpass(const std::string& name) {
    subpass_indices_.insert(std::make_pair(name, subpasses.size())); 
    subpasses.push_back(std::make_unique<RenderSubpass>(name, *this));
    return *subpasses.back();
}

void RenderPass::addSubpassDependency(const std::string& src, const std::string& dst, std::array<vk::PipelineStageFlags, 2> stage, std::array<vk::AccessFlags, 2> access) {
    final_dependencies.push_back({
        src == "external_subpass" ? VK_SUBPASS_EXTERNAL : getSubpassIndex(src),
        getSubpassIndex(dst),
        stage[0],
        stage[1],
        access[0],
        access[1]
    });
}

void RenderPass::build() {
    vk::RenderPassCreateInfo create_info;

    final_attachments.clear();
    final_attachments.reserve(attachments.size());
    for (const auto& attachment : attachments) {
        final_attachments.push_back(attachment.write_attachment);
    }

    final_subpasses.clear();
    final_subpasses.reserve(subpasses.size());
    for (const auto& subpass : subpasses) {
        final_subpasses.push_back(subpass->build());
    }

    create_info.setAttachmentCount(final_attachments.size())
        .setAttachments(final_attachments)
        .setSubpassCount(final_subpasses.size())
        .setSubpasses(final_subpasses)
        .setDependencyCount(final_dependencies.size())
        .setDependencies(final_dependencies);
    
    render_pass = manager->device->device.createRenderPass(create_info);
}

void RenderPass::update() {
    manager->device->device.destroyRenderPass(render_pass);
    build();
}

uint32_t RenderPass::getAttachmentIndex(const std::string& name) const {
    auto it = attachment_indices_.find(name);
    if (it == attachment_indices_.end()) {
        WEN_ERROR("Attachment \"{}\" not found", name)
    }
    return it->second; 
}

uint32_t RenderPass::getSubpassIndex(const std::string& name) const {
    auto it = subpass_indices_.find(name);
    if (it == subpass_indices_.end()) {
        WEN_ERROR("Subpass \"{}\" not found", name)
    }
    return it->second;
}

} // namespace wen