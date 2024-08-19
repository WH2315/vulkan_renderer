#pragma once

#include "resources/shader.hpp"

namespace wen {

class ShaderProgram {
    friend class RenderPipeline;

public:
    ShaderProgram() = default;
    ~ShaderProgram();

    ShaderProgram& attach(const std::shared_ptr<Shader>& shader);

private:
    std::shared_ptr<Shader> vert_shader_;
    std::shared_ptr<Shader> frag_shader_;
};

} // namespace wen