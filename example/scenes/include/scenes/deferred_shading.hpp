#pragma once

#include "scenes.hpp"
#include "camera.hpp"
#include <array>

class DeferredShading : public Scene {
public:
    struct Light {
        Light(std::shared_ptr<wen::Interface> interface);

        struct PointLight {
            alignas(16) glm::vec3 position;
            alignas(16) glm::vec3 color;
            alignas(4) float intensity;
        };

        struct LightUniform {
            alignas(16) PointLight lights[8];
            alignas(4) uint32_t light_count;
            alignas(4) int display_mode;
        };

        LightUniform* data;
        std::shared_ptr<wen::UniformBuffer> uniform_buffer;
    };

public:
    DeferredShading(std::shared_ptr<wen::Interface> interface) : Scene(interface) {}

    void initialize() override;
    void update(float ts, float w, float h) override;
    void render(float w, float h) override;
    void imgui() override;
    void destroy() override;

private:
    void refreshInputAttachments();

    int n_;
    std::vector<glm::vec3> offsets_;
    std::unique_ptr<Camera> camera_;
    std::unique_ptr<Light> light_;
    std::shared_ptr<wen::DescriptorSet> main_descriptor_set_;
    std::shared_ptr<wen::DescriptorSet> post_descriptor_set_;
    std::shared_ptr<wen::Sampler> gbuffer_sampler_;
    std::array<vk::ImageView, 3> last_input_views_{};
    std::shared_ptr<wen::ShaderProgram> main_shader_program_;
    std::shared_ptr<wen::ShaderProgram> post_shader_program_;
    std::shared_ptr<wen::NormalModel> model_;
    std::shared_ptr<wen::VertexBuffer> vertex_buffer_;
    std::shared_ptr<wen::IndexBuffer> index_buffer_;
    std::shared_ptr<wen::VertexBuffer> offsets_buffer_;
    std::shared_ptr<wen::RenderPipeline> main_render_pipeline_;
    std::shared_ptr<wen::RenderPipeline> post_render_pipeline_;
};