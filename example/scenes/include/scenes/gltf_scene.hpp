#pragma once

#include "scenes.hpp"
#include "camera.hpp"

struct Material {
    glm::vec3 albedo = glm::vec3(1.0f);
    float roughness = 0.3f;
    glm::vec3 specular_albedo = glm::vec3(1.0f);
    float specular_probability = 0.2f;
    glm::vec3 emissive_color = glm::vec3(1.0f, 0.9f, 0.8f);
    float emissive_intensity = 0.5f;
};

class GLTFScene : public Scene {
public:
    GLTFScene(std::shared_ptr<wen::Interface> interface) : Scene(std::move(interface)) {
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

    Material material_;

    std::shared_ptr<wen::SphereModel> model1_;
    std::shared_ptr<wen::NormalModel> model2_;
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