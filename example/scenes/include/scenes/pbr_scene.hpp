#pragma once

#include "scenes.hpp"
#include "camera.hpp"

struct PBRMaterial {
    PBRMaterial(wen::Interface& interface);

    struct MaterialUniform {
        alignas(16) glm::vec3 albedo;
        alignas(4) float metallic;
        alignas(4) float roughness;
        alignas(4) float ao;
    };

    MaterialUniform* data;
    std::shared_ptr<wen::UniformBuffer> uniform_buffer;
};

class PBRScene : public Scene {
public:
    struct Light {
        Light(wen::Interface& interface);

        struct PointLight {
            alignas(16) glm::vec3 position;
            alignas(16) glm::vec3 color;
        };

        struct LightUniform {
            alignas(16) glm::vec3 direction;
            alignas(16) glm::vec3 color;
            alignas(16) PointLight point_lights[3];
        };

        LightUniform* data;
        std::shared_ptr<wen::UniformBuffer> uniform_buffer;
    };

public:
    PBRScene(std::shared_ptr<wen::Interface> interface) : Scene(interface) {}

    void initialize() override;
    void update(float ts, float w, float h) override;
    void render(float w, float h) override;
    void imgui() override;
    void destroy() override;

private:
    std::unique_ptr<Camera> camera_;
    std::unique_ptr<Light> light_;
    std::unique_ptr<PBRMaterial> material_;
    std::shared_ptr<wen::GraphicsShaderProgram> shader_program_;
    std::shared_ptr<wen::NormalModel> model_;
    std::shared_ptr<wen::VertexBuffer> vertex_buffer_;
    std::shared_ptr<wen::IndexBuffer> index_buffer_;
    std::shared_ptr<wen::GraphicsRenderPipeline> render_pipeline_;
};