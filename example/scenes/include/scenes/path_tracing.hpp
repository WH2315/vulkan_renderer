#pragma once

#include "scenes.hpp"
#include "camera.hpp"

class PathTracing : public Scene {
public:
    PathTracing(std::shared_ptr<wen::Interface> interface)
        : Scene(std::move(interface)) {
        is_enable_ray_tracing = true;
    }

    void initialize() override;
    void update(float ts, float w, float h) override;
    void render(float w, float h) override;
    void imgui() override;
    void destroy() override;

    void recreateImageOutput();

private:
    int last_w = 0;
    int last_h = 0;

    int frame_index_ = 0;

    std::unique_ptr<Camera> camera_;

    std::shared_ptr<wen::GLTFScene> scene_;
    std::shared_ptr<wen::AccelerationStructure> as_;
    std::shared_ptr<wen::RayTracingInstance> rt_instance_;

    std::shared_ptr<wen::PushConstants> pcs_;
    std::shared_ptr<wen::DescriptorSet> rt_ds_;
    std::shared_ptr<wen::RayTracingShaderProgram> rt_sp_;
    std::shared_ptr<wen::RayTracingRenderPipeline> rt_rp_;
    std::shared_ptr<wen::DescriptorSet> image_ds_;
    std::shared_ptr<wen::GraphicsShaderProgram> graphics_sp_;
    std::shared_ptr<wen::GraphicsRenderPipeline> graphics_rp_;
    std::shared_ptr<wen::StorageImage> image_;
    std::shared_ptr<wen::Sampler> sampler_;
};