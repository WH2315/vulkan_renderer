#include "interface.hpp"

namespace wen {

Interface::Interface(const std::string& path) : path_(path) {
    shader_dir_ = path_ + "/shaders";
}

std::shared_ptr<Shader> Interface::loadShader(const std::string& filename, ShaderStage stage) {
    return std::make_shared<Shader>(shader_dir_ + "/" + filename, stage);
}

std::shared_ptr<ShaderProgram> Interface::createShaderProgram() {
    return std::make_shared<ShaderProgram>();
}

std::shared_ptr<RenderPipeline> Interface::createRenderPipeline(const std::shared_ptr<ShaderProgram>& shader_program) {
    return std::make_shared<RenderPipeline>(shader_program);
}

} // namespace wen