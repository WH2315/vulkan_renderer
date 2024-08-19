#pragma once

#include "resources/render_pass.hpp"
#include "renderer.hpp"
#include "resources/shader.hpp"
#include "resources/shader_program.hpp"
#include "resources/render_pipeline.hpp"

namespace wen {

class Interface {
public:
    Interface(const std::string& path);

    std::shared_ptr<RenderPass> createRenderPass();
    std::shared_ptr<Renderer> createRenderer(std::shared_ptr<RenderPass> render_pass);
    std::shared_ptr<Shader> loadShader(const std::string& filename, ShaderStage stage);
    std::shared_ptr<ShaderProgram> createShaderProgram();
    std::shared_ptr<RenderPipeline> createRenderPipeline(std::weak_ptr<Renderer> renderer, std::shared_ptr<ShaderProgram> shader_program, const std::string& subpass_name);

private:
    std::string path_;
    std::string shader_dir_;
};

} // namespace wen