#pragma once

#include "resources/shader_program.hpp"
#include "resources/vertex_input/vertex_input.hpp"
#include "resources/descriptor/descriptor_set.hpp"
#include "resources/push_constants/push_constants.hpp"
#include <memory>

namespace wen {

class RenderPipeline {
public:
    RenderPipeline() = default;
    virtual ~RenderPipeline();

protected:
    vk::PipelineShaderStageCreateInfo createShaderStage(vk::ShaderStageFlagBits stage, vk::ShaderModule module);
    void createPipelineLayout();

public:
    vk::PipelineLayout pipeline_layout;
    vk::Pipeline pipeline;
    std::vector<std::optional<std::shared_ptr<DescriptorSet>>> descriptor_sets;
    std::optional<std::shared_ptr<PushConstants>> push_constants;
};

template <class RenderPipelineClass, typename Options>
class RenderPipelineTemplate : public RenderPipeline {
public:
    ~RenderPipelineTemplate() override = default;


    void setDescriptorSet(std::shared_ptr<DescriptorSet> descriptor_set, uint32_t index = 0) {
        if (index + 1 > descriptor_sets.size()) {
            descriptor_sets.resize(index + 1);
        }
        descriptor_sets[index] = std::move(descriptor_set);
    }

    void setPushConstants(std::shared_ptr<PushConstants> push_constants) {
        this->push_constants = std::move(push_constants);
    }

    virtual void compile(const Options& options) = 0;
};

struct GraphicsRenderPipelineOptions {
    vk::PolygonMode polygon_mode = vk::PolygonMode::eFill;
    float line_width = 1.0f;
    vk::Bool32 depth_test_enable = false;
    std::vector<vk::DynamicState> dynamic_states = {};
};

class Renderer;
class GraphicsRenderPipeline : public RenderPipelineTemplate<GraphicsRenderPipeline, GraphicsRenderPipelineOptions> {
public:
    GraphicsRenderPipeline(std::weak_ptr<Renderer> renderer, const std::shared_ptr<GraphicsShaderProgram>& shader_program, const std::string& subpass_name);
    ~GraphicsRenderPipeline() override;

    void setVertexInput(std::shared_ptr<VertexInput> vertex_input);
    void compile(const GraphicsRenderPipelineOptions& options) override;

    vk::PipelineBindPoint bind_point = vk::PipelineBindPoint::eGraphics;

private:
    std::weak_ptr<Renderer> renderer_;
    std::shared_ptr<GraphicsShaderProgram> shader_program_;
    std::string subpass_name_;
    std::shared_ptr<VertexInput> vertex_input_;
};

} // namespace wen