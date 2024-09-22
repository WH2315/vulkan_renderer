#include "resources/render_pipeline.hpp"
#include "manager.hpp"

namespace wen {

RenderPipeline::RenderPipeline(std::weak_ptr<Renderer> renderer, const std::shared_ptr<ShaderProgram>& shader_program, const std::string& subpass_name)
    : renderer_(renderer), shader_program_(shader_program), subpass_name_(subpass_name) {}

RenderPipeline::~RenderPipeline() {
    descriptor_sets.clear();
    manager->device->device.destroyPipeline(pipeline);
    manager->device->device.destroyPipelineLayout(pipeline_layout);
    renderer_.reset();
    shader_program_.reset();
    subpass_name_.clear();
}

void RenderPipeline::setVertexInput(std::shared_ptr<VertexInput> vertex_input) {
    vertex_input_ = std::move(vertex_input);
}

void RenderPipeline::setDescriptorSet(std::shared_ptr<DescriptorSet> descriptor_set, uint32_t index) {
    if (index + 1 > descriptor_sets.size()) {
        descriptor_sets.resize(index + 1);
    }
    descriptor_sets[index] = std::move(descriptor_set);
}

void RenderPipeline::compile(const RenderPipelineOptions& options) {
    // 1. shader stages
    std::vector<vk::PipelineShaderStageCreateInfo> shader_stages = {
        createShaderStage(vk::ShaderStageFlagBits::eVertex, shader_program_->vert_shader_->module.value()),
        createShaderStage(vk::ShaderStageFlagBits::eFragment, shader_program_->frag_shader_->module.value())
    };

    // 2. vertex input
    vk::PipelineVertexInputStateCreateInfo vertex_input = {};
    vertex_input.setVertexAttributeDescriptions(vertex_input_->attribute_descriptions_)
        .setVertexBindingDescriptions(vertex_input_->binding_descriptions_);
    
    // 3. input assembly
    vk::PipelineInputAssemblyStateCreateInfo input_assembly = {};
    input_assembly.setPrimitiveRestartEnable(false)
        .setTopology(vk::PrimitiveTopology::eTriangleList);
    
    // 4. viewport
    vk::PipelineViewportStateCreateInfo viewport = {};
    auto width = renderer_config->getWidth(), height = renderer_config->getHeight();
    auto w = static_cast<float>(width), h = static_cast<float>(height);
    vk::Viewport view(0.0f, 0.0f, w, h, 0.0f, 1.0f);
    vk::Rect2D scissor({0, 0}, {width, height});
    viewport.setViewportCount(1)
        .setViewports(view)
        .setScissorCount(1)
        .setScissors(scissor);
    
    // 5. rasterizer
    vk::PipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.setDepthClampEnable(true)
        .setRasterizerDiscardEnable(false)
        .setPolygonMode(options.polygon_mode)
        .setLineWidth(options.line_width)
        .setCullMode(vk::CullModeFlagBits::eNone)
        .setFrontFace(vk::FrontFace::eClockwise)
        .setDepthBiasEnable(false)
        .setDepthBiasConstantFactor(0.0f)
        .setDepthBiasClamp(false)
        .setDepthBiasSlopeFactor(0.0f);
    
    // 6. multisample
    vk::PipelineMultisampleStateCreateInfo multisample = {};
    multisample.setSampleShadingEnable(true)
        .setRasterizationSamples(renderer_config->getSampleCount())
        .setMinSampleShading(0.2f)
        .setPSampleMask(nullptr)
        .setAlphaToCoverageEnable(false)
        .setAlphaToOneEnable(false);

    // 7. depth and stencil
    vk::PipelineDepthStencilStateCreateInfo depth_stencil = {};
    depth_stencil.setDepthTestEnable(options.depth_test_enable)
        .setDepthWriteEnable(true)
        .setDepthCompareOp(vk::CompareOp::eLess)
        .setDepthBoundsTestEnable(false)
        .setStencilTestEnable(false)
        .setFront({})
        .setBack({});

    // 8. color blending
    vk::PipelineColorBlendStateCreateInfo color_blend = {};
    auto locked_renderer = renderer_.lock();
    uint32_t subpass_index = locked_renderer->render_pass->getSubpassIndex(subpass_name_);
    auto subpass = *locked_renderer->render_pass->subpasses[subpass_index];
    color_blend.setLogicOpEnable(false)
        .setAttachments(subpass.color_blend_attachments)
        .setBlendConstants({0.0f, 0.0f, 0.0f, 0.0f});
    
    // 9. dynamic state
    vk::PipelineDynamicStateCreateInfo dynamic = {};
    dynamic.setDynamicStateCount(options.dynamic_states.size())
        .setDynamicStates(options.dynamic_states);
    
    // create pipeline layout
    createPipelineLayout();

    // pipeline
    vk::GraphicsPipelineCreateInfo create_info;
    create_info.setStageCount(shader_stages.size())
        .setPStages(shader_stages.data())
        .setPVertexInputState(&vertex_input)
        .setPInputAssemblyState(&input_assembly)
        .setPTessellationState(nullptr)
        .setPViewportState(&viewport)
        .setPRasterizationState(&rasterizer)
        .setPMultisampleState(&multisample)
        .setPDepthStencilState(&depth_stencil)
        .setPColorBlendState(&color_blend)
        .setPDynamicState(&dynamic)
        .setLayout(pipeline_layout)
        .setRenderPass(locked_renderer->render_pass->render_pass)
        .setSubpass(subpass_index)
        .setBasePipelineHandle(nullptr)
        .setBasePipelineIndex(-1);
    
    locked_renderer.reset();

    pipeline = manager->device->device.createGraphicsPipeline(nullptr, create_info).value;
}

vk::PipelineShaderStageCreateInfo RenderPipeline::createShaderStage(vk::ShaderStageFlagBits stage, vk::ShaderModule module) {
    vk::PipelineShaderStageCreateInfo create_info;
    create_info.setPSpecializationInfo(nullptr)
        .setStage(stage)
        .setModule(module)
        .setPName("main");
    return create_info;
}

void RenderPipeline::createPipelineLayout() {
    vk::PipelineLayoutCreateInfo create_info;

    std::vector<vk::DescriptorSetLayout> descriptor_set_layouts = {};
    descriptor_set_layouts.reserve(descriptor_sets.size());
    for (const auto& descriptor_set : descriptor_sets) {
        if (descriptor_set.has_value()) {
            descriptor_set_layouts.push_back(descriptor_set.value()->descriptor_layout_);
        }
    }

    create_info.setSetLayouts(descriptor_set_layouts)
        .setPushConstantRanges(nullptr);
    
    pipeline_layout = manager->device->device.createPipelineLayout(create_info);
}

} // namespace wen