#pragma once

#include "resources/shader.hpp"
#include "resources/shader_program.hpp"
#include "resources/render_pipeline.hpp"

namespace wen {

class Interface {
public:
    Interface(const std::string& path);

    std::shared_ptr<Shader> loadShader(const std::string& filename, ShaderStage stage);
    std::shared_ptr<ShaderProgram> createShaderProgram();
    std::shared_ptr<RenderPipeline> createRenderPipeline(const std::shared_ptr<ShaderProgram>& shader_program);

private:
    std::string path_;
    std::string shader_dir_;
};

} // namespace wen