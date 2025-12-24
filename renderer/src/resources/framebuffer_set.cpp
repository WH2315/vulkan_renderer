#include "resources/framebuffer_set.hpp"
#include "renderer.hpp"
#include "manager.hpp"

namespace wen {

FramebufferSet::FramebufferSet(const Renderer& renderer) {
    uint32_t count = renderer.render_pass->attachments.size();
    uint32_t final_count = renderer.render_pass->final_attachments.size();

    attachments.resize(final_count);
    for (auto [name, index] : renderer.render_pass->getAttachmentIndices()) {
        auto attachment = renderer.render_pass->attachments[index];
        attachments[index] = new Attachment(
            attachment.attachment,
            attachment.usage,
            attachment.aspect
        );
        if (index == 0 || index == 1) continue;
        if (renderer_config->msaa()) {
            auto resolve_attachment = renderer.render_pass->resolve_attachments[index - 1];
            attachments[renderer.render_pass->getAttachmentIndex(name, true)] = new Attachment(
                resolve_attachment.attachment,
                attachment.usage,
                attachment.aspect
            );
        }
    }

    std::vector<vk::ImageView> image_views(final_count);
    for (uint32_t i = 0; i < final_count; i++) {
        if (renderer_config->msaa() && i == count) continue;
        image_views[i] = attachments[i]->image_view;
    }
    for (auto image_view : manager->swapchain->image_views) {
        image_views[renderer_config->msaa() ? count : 0] = image_view;
        framebuffers.push_back(std::make_unique<Framebuffer>(renderer, image_views));
    }
}

FramebufferSet::~FramebufferSet() {
    for (auto attachment : attachments) {
        delete attachment;
    }
    attachments.clear();
    framebuffers.clear();
}

} // namespace wen