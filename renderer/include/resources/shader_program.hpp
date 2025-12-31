#pragma once

#include "resources/shader.hpp"
#include <memory>

namespace wen {

class GraphicsShaderProgram {
    friend class GraphicsRenderPipeline;

public:
    GraphicsShaderProgram() = default;
    ~GraphicsShaderProgram();

    GraphicsShaderProgram& attach(const std::shared_ptr<Shader>& shader);

private:
    std::shared_ptr<Shader> vert_shader_;
    std::shared_ptr<Shader> frag_shader_;
};

} // namespace wen