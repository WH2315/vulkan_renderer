#include "resources/framebuffer_set.hpp"
#include "renderer.hpp"
#include "manager.hpp"

namespace wen {

FramebufferSet::FramebufferSet(const Renderer& renderer) {
    uint32_t count = renderer.render_pass->final_attachments.size();
    attachments.resize(count);

    for (auto [name, index] : renderer.render_pass->getAttachmentIndices()) {
        auto attachment = renderer.render_pass->attachments[index];
        attachments[index] = new Attachment(
            attachment.write_attachment,
            attachment.write_usage,
            attachment.aspect
        );
    }

    std::vector<vk::ImageView> image_views;
    for (auto image_view : manager->swapchain->image_views) {
        image_views.clear();
        image_views.push_back(image_view);
        for (uint32_t i = 1; i < count; i++) {
            image_views.push_back(attachments[i]->image_view);
        }
        framebuffers.emplace_back(std::make_unique<Framebuffer>(renderer, image_views));
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