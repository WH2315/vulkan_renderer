#include "ray_tracing/render_pipeline.hpp"
#include "base/utils.hpp"
#include "manager.hpp"

namespace wen {

RayTracingRenderPipeline::RayTracingRenderPipeline(const std::shared_ptr<RayTracingShaderProgram>& shader_program)
    : shader_program_(shader_program) {}

RayTracingRenderPipeline::~RayTracingRenderPipeline() {
    shader_program_.reset();
    buffer_.reset();
}

void RayTracingRenderPipeline::compile(const RayTracingRenderPipelineOptions& options) {
    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shader_groups;
    std::vector<vk::PipelineShaderStageCreateInfo> shader_stages;
    shader_groups.reserve(1 + shader_program_->miss_shaders_.size() + shader_program_->hit_groups_.size());
    shader_stages.reserve(1 + shader_program_->miss_shaders_.size() + shader_program_->hit_shader_count_);

    shader_groups.emplace_back()
        .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
        .setGeneralShader(shader_stages.size());
    shader_stages.push_back(
        createShaderStage(
            vk::ShaderStageFlagBits::eRaygenKHR,
            shader_program_->raygen_shader_->module.value()
        )
    );
    for (const auto& miss_shader : shader_program_->miss_shaders_) {
        shader_groups.emplace_back()
            .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
            .setGeneralShader(shader_stages.size());
        shader_stages.push_back(
            createShaderStage(
                vk::ShaderStageFlagBits::eMissKHR,
                miss_shader->module.value()
            )
        );
    }
    for (const auto& hit_group : shader_program_->hit_groups_) {
        auto& shader_group = shader_groups.emplace_back();
        shader_group.setClosestHitShader(shader_stages.size());
        shader_stages.push_back(
            createShaderStage(
                vk::ShaderStageFlagBits::eClosestHitKHR,
                hit_group.closest_hit_shader->module.value()
            )
        );
        if (hit_group.intersection_shader.has_value()) {
            shader_group.setType(vk::RayTracingShaderGroupTypeKHR::eProceduralHitGroup)
                .setIntersectionShader(shader_stages.size());
            shader_stages.push_back(
                createShaderStage(
                    vk::ShaderStageFlagBits::eIntersectionKHR,
                    hit_group.intersection_shader.value()->module.value()
                )
            );
        } else {
            shader_group.setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup);
        }
    }

    createPipelineLayout();

    vk::RayTracingPipelineCreateInfoKHR rt_pipeline_ci{};
    rt_pipeline_ci.setStages(shader_stages)
        .setGroups(shader_groups)
        .setMaxPipelineRayRecursionDepth(options.max_ray_recursion_depth)
        .setLayout(pipeline_layout);
    pipeline = manager->device->device.createRayTracingPipelineKHR({}, {}, rt_pipeline_ci, nullptr, manager->dispatcher).value;

    vk::PhysicalDeviceProperties2 properties = {};
    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rt_pipeline_properties = {};
    properties.pNext = &rt_pipeline_properties;
    manager->device->physical_device.getProperties2(&properties);

    auto align_address = [](uint32_t size, uint32_t align) {
        return (size + (align - 1)) & ~(align - 1);
    };
    uint32_t handle_size = rt_pipeline_properties.shaderGroupHandleSize;
    // 着色器绑定表 (缓存) 需要开头的组已经完成对齐并且组中的句柄也已经对齐完成
    uint32_t handle_size_aligned = align_address(handle_size, rt_pipeline_properties.shaderGroupHandleAlignment);
    uint32_t base_alignment = rt_pipeline_properties.shaderGroupBaseAlignment;
    raygen_region_.stride = align_address(handle_size, base_alignment);
    raygen_region_.size = raygen_region_.stride;
    miss_region_.stride = handle_size_aligned;
    miss_region_.size = align_address(shader_program_->miss_shaders_.size() * handle_size_aligned, base_alignment);
    hit_region_.stride = handle_size_aligned;
    hit_region_.size = align_address(shader_program_->hit_groups_.size() * handle_size_aligned, base_alignment);

    std::vector<uint8_t> handles(handle_size * shader_groups.size());
    auto res = manager->device->device.getRayTracingShaderGroupHandlesKHR(pipeline, 0, shader_groups.size(), handles.size(), handles.data(), manager->dispatcher);
    assert(res == vk::Result::eSuccess);
    // 分配用于存储着色器绑定表的缓存
    buffer_ = std::make_unique<Buffer>(
        raygen_region_.size + miss_region_.size + hit_region_.size,
        vk::BufferUsageFlagBits::eShaderDeviceAddress | vk::BufferUsageFlagBits::eShaderBindingTableKHR,
        VMA_MEMORY_USAGE_AUTO,
        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT
    );
    // 获取每组的着色器绑定表
    auto address = getBufferAddress(buffer_->buffer);
    raygen_region_.setDeviceAddress(address);
    miss_region_.setDeviceAddress(address + raygen_region_.size);
    hit_region_.setDeviceAddress(address + raygen_region_.size + miss_region_.size);

    auto* ptr = static_cast<uint8_t*>(buffer_->map());
    auto get_handle = [&](int i) {
        return handles.data() + i * handle_size;
    };
    memcpy(ptr, get_handle(0), handle_size);
    ptr += raygen_region_.size;
    for (uint32_t i = 0; i < shader_program_->miss_shaders_.size(); i++) {
        memcpy(ptr + i * handle_size_aligned, get_handle(1 + i), handle_size);
    }
    ptr += miss_region_.size;
    for (uint32_t i = 0; i < shader_program_->hit_groups_.size(); i++) {
        memcpy(ptr + i * handle_size_aligned, get_handle(1 + shader_program_->miss_shaders_.size() + i), handle_size);
    }
    buffer_->unmap();
}

} // namespace wen