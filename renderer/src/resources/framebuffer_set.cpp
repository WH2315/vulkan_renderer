#include "resources/framebuffer_set.hpp"
#include "renderer.hpp"
#include "manager.hpp"

namespace wen {

FramebufferSet::FramebufferSet(const Renderer& renderer) {
    uint32_t count = renderer.render_pass->final_attachments.size();
    std::vector<vk::ImageView> image_views(count);
    attachments.resize(count);

    for (auto image_view : manager->swapchain->image_views) {
        image_views[0] = image_view;
        framebuffers.emplace_back(std::make_unique<Framebuffer>(renderer, image_views));
    }
}

} // namespace wen