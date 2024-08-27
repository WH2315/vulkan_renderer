#pragma once

#include "basic/framebuffer.hpp"

namespace wen {

struct FramebufferSet {
    FramebufferSet(const Renderer& renderer);

    std::vector<Attachment> attachments;
    std::vector<std::unique_ptr<Framebuffer>> framebuffers;
};

} // namespace wen