#include "interface.hpp"

namespace wen {

Interface::Interface(const std::string& path) : path_(path) {
    shader_dir_ = path_ + "/shaders";
}

std::shared_ptr<RenderPass> Interface::createRenderPass() {
    return std::make_shared<RenderPass>();
}

std::shared_ptr<Renderer> Interface::createRenderer(std::shared_ptr<RenderPass> render_pass) {
    return std::make_shared<Renderer>(render_pass);
}

std::shared_ptr<Shader> Interface::loadShader(const std::string& filename, ShaderStage stage) {
    return std::make_shared<Shader>(shader_dir_ + "/" + filename, stage);
}

std::shared_ptr<ShaderProgram> Interface::createShaderProgram() {
    return std::make_shared<ShaderProgram>();
}

std::shared_ptr<RenderPipeline> Interface::createRenderPipeline(std::weak_ptr<Renderer> renderer, std::shared_ptr<ShaderProgram> shader_program, const std::string& subpass_name) {
    return std::make_shared<RenderPipeline>(renderer, shader_program, subpass_name);
}

} // namespace wen