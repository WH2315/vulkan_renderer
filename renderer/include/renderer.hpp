#pragma once

#include "resources/render_pass.hpp"

namespace wen {

class Renderer {
public:
    Renderer(std::shared_ptr<RenderPass> render_pass);
    ~Renderer();

public:
    std::shared_ptr<RenderPass> render_pass;
};

} // namespace wen