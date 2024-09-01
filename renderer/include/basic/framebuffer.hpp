#pragma once

#include "resources/image.hpp"

namespace wen {

struct Attachment {
    Attachment(vk::AttachmentDescription description, vk::ImageUsageFlags usage, vk::ImageAspectFlags aspect);
    ~Attachment();

    std::unique_ptr<Image> image;
    vk::ImageView image_view = nullptr;
};

class Renderer;
class Framebuffer {
    friend class Renderer;

public:
    Framebuffer(const Renderer& renderer, const std::vector<vk::ImageView>& image_views);
    ~Framebuffer();

private:
    vk::Framebuffer framebuffer_;
};

} // namespace wen