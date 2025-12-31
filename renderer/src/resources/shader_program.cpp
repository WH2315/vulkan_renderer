#include "resources/shader_program.hpp"

namespace wen {

GraphicsShaderProgram& GraphicsShaderProgram::attach(const std::shared_ptr<Shader>& shader) {
    if (shader->stage == ShaderStage::eVertex) {
        vert_shader_ = shader;
    } else if (shader->stage == ShaderStage::eFragment) {
        frag_shader_ = shader;
    }
    return *this;
}

GraphicsShaderProgram::~GraphicsShaderProgram() {
    if (vert_shader_) {
        vert_shader_.reset();
    }
    if (frag_shader_) {
        frag_shader_.reset();
    }
}

} // namespace wen