#include "renderer.hpp"

namespace wen {

Renderer::Renderer(std::shared_ptr<RenderPass> render_pass) {
    this->render_pass = render_pass;
}

Renderer::~Renderer() {
    render_pass.reset();
}

} // namespace wen