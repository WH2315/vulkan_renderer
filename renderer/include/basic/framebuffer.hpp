#pragma once

#include <vulkan/vulkan.hpp>

namespace wen {

struct Attachment {
    vk::ImageView view;
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