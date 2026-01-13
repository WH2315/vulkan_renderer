#pragma once

#include "scenes.hpp"
#include "camera.hpp"

struct MicrofacetTheoryMaterial {
    enum class Type : int {
        // 电解质
        eDielectric = 0,
        // 导体
        eConductor = 1
    };
    glm::vec3 albedo;
    Type type;
    // 导体的折射率为复数，IOR为实部，K为虚部
    float IOR;
    float K;
    // 水平方向粗糙度
    float alpha_x;
    // 垂直方向粗糙度
    float alpha_y;
};

class MicrofacetTheory : public Scene {
public:
    MicrofacetTheory(std::shared_ptr<wen::Interface> interface)
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

    MicrofacetTheoryMaterial dielectric_material_{};
    MicrofacetTheoryMaterial conductor_material_{};
    std::shared_ptr<wen::SphereModel> sphere_;

    MicrofacetTheoryMaterial bunny_d1_material_{};
    MicrofacetTheoryMaterial bunny_d2_material_{};
    MicrofacetTheoryMaterial bunny_c_material_{};
    std::shared_ptr<wen::NormalModel> bunny_;

    MicrofacetTheoryMaterial outter_d_material_{};
    MicrofacetTheoryMaterial outter_c_material_{};
    MicrofacetTheoryMaterial inner_material_{};
    std::shared_ptr<wen::NormalModel> outter_;
    std::shared_ptr<wen::NormalModel> inner_;

    MicrofacetTheoryMaterial dragon_material_{};
    std::shared_ptr<wen::NormalModel> dragon_;

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