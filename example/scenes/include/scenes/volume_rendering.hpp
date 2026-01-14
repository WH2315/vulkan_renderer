#pragma once

#include "scenes.hpp"
#include "camera.hpp"

enum class SphereType : int {
    eGround = 0,  // 地面
    eUniform = 1, // 均匀介质
    ePerlin = 2,  // Perlin噪声介质
    eCloud = 3,   // 云介质
    eSmoke = 4    // 烟雾介质
};

struct VolumeRenderingArgs {
    // Uniform
    float sigma_maj_uniform = 0.8;
    float sigma_a_uniform = 0.5;
    float sigma_s_uniform = 0.2;
    float g_uniform = 0;

    // Perlin
    float sigma_maj_perlin = 7.8;
    float sigma_a_perlin = 5;
    float sigma_s_perlin = 0.16;
    float g_perlin = 0;
    float L = 3.376;
    float H = 0.728;
    float freq = 0.506;
    int OCT = 3;

    // Cloud
    float sigma_maj_cloud = 10;
    float sigma_a_cloud = 0.4;
    float sigma_s_cloud = 9.4;
    float g_cloud = -0.9;

    // Smoke
    float sigma_maj_smoke = 12;
    float sigma_a_smoke = 9.59;
    float sigma_s_smoke = 1.76;
    float g_smoke = -0.9;
};

class VolumeRendering : public Scene {
public:
    VolumeRendering(std::shared_ptr<wen::Interface> interface)
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

    std::shared_ptr<wen::SphereModel> sphere_;
    std::shared_ptr<wen::UniformBuffer> volume_data_args_;

    std::shared_ptr<wen::VolumeData> volume_data_cloud_;
    std::shared_ptr<wen::VertexBuffer> volume_buffer_cloud_;
    std::shared_ptr<wen::VolumeData> volume_data_smoke_;
    std::shared_ptr<wen::VertexBuffer> volume_buffer_smoke_;

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