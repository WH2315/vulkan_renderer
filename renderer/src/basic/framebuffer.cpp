#include "basic/framebuffer.hpp"
#include "renderer.hpp"
#include "manager.hpp"

namespace wen {

Framebuffer::Framebuffer(const Renderer& renderer, vk::ImageView image_view) {
    vk::FramebufferCreateInfo create_info;
    create_info.setRenderPass(renderer.render_pass->render_pass)
        .setWidth(renderer_config->getWidth())
        .setHeight(renderer_config->getHeight())
        .setLayers(1)
        .setAttachments(image_view);
    framebuffer_ = manager->device->device.createFramebuffer(create_info);
}

Framebuffer::~Framebuffer() {
    manager->device->device.destroyFramebuffer(framebuffer_);
}

} // namespace wen