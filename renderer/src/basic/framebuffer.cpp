#include "basic/framebuffer.hpp"
#include "base/utils.hpp"
#include "renderer.hpp"
#include "manager.hpp"

namespace wen {

Attachment::Attachment(vk::AttachmentDescription description, vk::ImageUsageFlags usage, vk::ImageAspectFlags aspect) {
    image = std::make_unique<Image>(
        renderer_config->getWidth(),
        renderer_config->getHeight(),
        description.format,
        usage,
        description.samples,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT
    );
    image_view = createImageView(image->image, description.format, aspect, 1);
}

Attachment::~Attachment() {
    image.reset();
    manager->device->device.destroyImageView(image_view);
}

Framebuffer::Framebuffer(const Renderer& renderer, const std::vector<vk::ImageView>& image_views) {
    vk::FramebufferCreateInfo create_info;
    create_info.setRenderPass(renderer.render_pass->render_pass)
        .setWidth(renderer_config->getWidth())
        .setHeight(renderer_config->getHeight())
        .setLayers(1)
        .setAttachments(image_views);
    framebuffer_ = manager->device->device.createFramebuffer(create_info);
}

Framebuffer::~Framebuffer() {
    manager->device->device.destroyFramebuffer(framebuffer_);
}

} // namespace wen