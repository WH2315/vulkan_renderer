#pragma once

#include "resources/shader_program.hpp"
#include "renderer.hpp"

namespace wen {

struct RenderPipelineOptions {
    vk::Bool32 depth_test_enable = false;
    std::vector<vk::DynamicState> dynamic_states = {};
};

class RenderPipeline {
public:
    RenderPipeline(std::weak_ptr<Renderer> renderer, const std::shared_ptr<ShaderProgram>& shader_program, const std::string& subpass_name);
    ~RenderPipeline();

    void compile(const RenderPipelineOptions& options = {});

public:
    vk::PipelineLayout pipeline_layout;
    vk::Pipeline pipeline;

private:
    std::weak_ptr<Renderer> renderer_;
    std::shared_ptr<ShaderProgram> shader_program_;
    std::string subpass_name_;

private:
    vk::PipelineShaderStageCreateInfo createShaderStage(vk::ShaderStageFlagBits stage, vk::ShaderModule module);
    void createPipelineLayout();
};

} // namespace wen