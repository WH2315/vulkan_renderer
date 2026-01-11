#pragma once

#include "resources/render_pipeline.hpp"
#include "ray_tracing/shader_program.hpp"

namespace wen {

struct RayTracingRenderPipelineOptions {
    uint32_t max_ray_recursion_depth = 1;
};

class RayTracingRenderPipeline : public RenderPipelineTemplate<RayTracingRenderPipeline, RayTracingRenderPipelineOptions> {
    friend class Renderer;

public:
    RayTracingRenderPipeline(const std::shared_ptr<RayTracingShaderProgram>& shader_program);
    ~RayTracingRenderPipeline() override;

    void compile(const RayTracingRenderPipelineOptions& options) override;

    vk::PipelineBindPoint bind_point = vk::PipelineBindPoint::eRayTracingKHR;

private:
    std::shared_ptr<RayTracingShaderProgram> shader_program_;
    std::unique_ptr<Buffer> buffer_;
    vk::StridedDeviceAddressRegionKHR raygen_region_{};
    vk::StridedDeviceAddressRegionKHR miss_region_{};
    vk::StridedDeviceAddressRegionKHR hit_region_{};
    vk::StridedDeviceAddressRegionKHR callable_region_{};
};

} // namespace wen