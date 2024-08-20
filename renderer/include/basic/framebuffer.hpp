#pragma once

#include <vulkan/vulkan.hpp>

namespace wen {

class Renderer;
class Framebuffer {
    friend class Renderer;

public:
    Framebuffer(const Renderer& renderer, vk::ImageView image_view);
    ~Framebuffer();

private:
    vk::Framebuffer framebuffer_;
};

} // namespace wen