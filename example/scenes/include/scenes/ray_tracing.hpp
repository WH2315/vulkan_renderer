#pragma once

#include "scenes.hpp"
#include "camera.hpp"

class RayTracing : public Scene {
public:
    struct Info {
        alignas(16) glm::vec2 window_size;
        alignas(16) glm::vec3 clear_color;
    };

    RayTracing(std::shared_ptr<wen::Interface> interface)
        : Scene(std::move(interface)) {
        is_enable_ray_tracing = true;
    }

    void initialize() override;
    void update(float ts, float w, float h) override;
    void render(float w, float h) override;
    void imgui() override;
    void destroy() override;

    void createAccelerationStructure();
    void recreateRayTracingOutput();

private:
    float last_w, last_h;

    std::unique_ptr<Camera> camera_;

    Info* info_;
    std::shared_ptr<wen::UniformBuffer> info_uniform_;
    std::shared_ptr<wen::DescriptorSet> shader_descriptor_set_;
    glm::vec3 point_light_position_;
    float light_rotation_time_ = 0.0f;
    bool light_rotation_enabled_ = false;
    std::shared_ptr<wen::PushConstants> push_constants_;

    std::shared_ptr<wen::NormalModel> model1_;
    std::shared_ptr<wen::NormalModel> model2_;
    std::shared_ptr<wen::NormalModel> model3_;
    std::shared_ptr<wen::VertexBuffer> vertex_buffer_;
    std::shared_ptr<wen::IndexBuffer> index_buffer_;

    // no ray tracing
    std::shared_ptr<wen::GraphicsShaderProgram> shader_program_;
    std::shared_ptr<wen::GraphicsRenderPipeline> render_pipeline_;

    // ray tracing
    std::shared_ptr<wen::RayTracingInstance> ray_tracing_instance_;
    std::map<uint32_t, std::vector<std::tuple<glm::vec3, glm::vec3, float, float>>>
        transform_infos_;

    std::shared_ptr<wen::DescriptorSet> ray_tracing_descriptor_set_;
    std::shared_ptr<wen::RayTracingShaderProgram> ray_tracing_shader_program_;
    std::shared_ptr<wen::RayTracingRenderPipeline> ray_tracing_render_pipeline_;
    std::shared_ptr<wen::DescriptorSet> image_descriptor_set_;
    std::shared_ptr<wen::GraphicsShaderProgram> graphics_shader_program_;
    std::shared_ptr<wen::GraphicsRenderPipeline> graphics_render_pipeline_;
    std::shared_ptr<wen::StorageImage> image_;
    std::shared_ptr<wen::Sampler> sampler_;
};