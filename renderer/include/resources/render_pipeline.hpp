#pragma once

#include "resources/shader_program.hpp"
#include <vulkan/vulkan.hpp>

namespace wen {

struct RenderPipelineOptions {
    vk::Bool32 depth_test_enable = false;
    std::vector<vk::DynamicState> dynamic_states = {};
};

class RenderPipeline {
public:
    RenderPipeline(const std::shared_ptr<ShaderProgram>& shader_program);
    ~RenderPipeline();

    void compile(const RenderPipelineOptions& options = {});

public:
    vk::PipelineLayout pipeline_layout;

private:
    std::shared_ptr<ShaderProgram> shader_program_;

private:
    vk::PipelineShaderStageCreateInfo createShaderStage(vk::ShaderStageFlagBits stage, vk::ShaderModule module);
    void createPipelineLayout();
};

} // namespace wen