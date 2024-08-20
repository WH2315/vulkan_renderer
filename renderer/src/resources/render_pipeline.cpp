#include "resources/render_pipeline.hpp"
#include "manager.hpp"

namespace wen {

RenderPipeline::RenderPipeline(std::weak_ptr<Renderer> renderer, const std::shared_ptr<ShaderProgram>& shader_program, const std::string& subpass_name) {
    renderer_ = renderer;
    shader_program_ = shader_program;
    subpass_name_ = subpass_name;
}

RenderPipeline::~RenderPipeline() {
    manager->device->device.destroyPipeline(pipeline);
    manager->device->device.destroyPipelineLayout(pipeline_layout);
    renderer_.reset();
    shader_program_.reset();
    subpass_name_.clear();
}

void RenderPipeline::compile(const RenderPipelineOptions& options) {
    // 1. shader stages
    std::vector<vk::PipelineShaderStageCreateInfo> shader_stages = {
        createShaderStage(vk::ShaderStageFlagBits::eVertex, shader_program_->vert_shader_->module.value()),
        createShaderStage(vk::ShaderStageFlagBits::eFragment, shader_program_->frag_shader_->module.value())
    };

    // 2. vertex input
    vk::PipelineVertexInputStateCreateInfo vertex_input = {};
    vertex_input.setVertexAttributeDescriptions(nullptr)
        .setVertexBindingDescriptions(nullptr);
    
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
        .setPolygonMode(vk::PolygonMode::eFill)
        .setLineWidth(1.0f)
        .setCullMode(vk::CullModeFlagBits::eNone)
        .setFrontFace(vk::FrontFace::eClockwise)
        .setDepthBiasEnable(false)
        .setDepthBiasConstantFactor(0.0f)
        .setDepthBiasClamp(false)
        .setDepthBiasSlopeFactor(0.0f);
    
    // 6. multisample
    vk::PipelineMultisampleStateCreateInfo multisample = {};
    multisample.setSampleShadingEnable(false)
        .setRasterizationSamples(vk::SampleCountFlagBits::e1)
        .setMinSampleShading(1.0f)
        .setPSampleMask(nullptr)
        .setAlphaToCoverageEnable(false)
        .setAlphaToOneEnable(false);

    // 7. depth and stencil
    vk::PipelineDepthStencilStateCreateInfo depth_stencil = {};
    depth_stencil.setDepthTestEnable(options.depth_test_enable);

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

    create_info.setSetLayouts(descriptor_set_layouts)
        .setPushConstantRanges(nullptr);
    
    pipeline_layout = manager->device->device.createPipelineLayout(create_info);
}

} // namespace wen