#pragma once

#include "scenes.hpp"
#include "camera.hpp"

struct RayMarchingInfo {
    RayMarchingInfo(wen::Interface& interface);

    struct RayMarchingUniform {
        alignas(16) glm::vec2 window_size;
        alignas(4) int max_steps;
        alignas(4) float max_dist;
        alignas(4) float epsillon_dist;
        alignas(16) glm::vec4 sphere;
        alignas(16) glm::vec3 light;
        alignas(4) float intensity;
    };

    RayMarchingUniform* data;
    std::shared_ptr<wen::UniformBuffer> uniform_buffer;
};

class RayMarching : public Scene {
public:
    RayMarching(std::shared_ptr<wen::Interface> interface) : Scene(interface) {}

    void initialize() override;
    void update(float ts, float w, float h) override;
    void render(float w, float h) override;
    void imgui() override;
    void destroy() override;

private:
    std::unique_ptr<Camera> camera_;
    std::unique_ptr<RayMarchingInfo> info_;
    std::shared_ptr<wen::ShaderProgram> shader_program_;
    std::shared_ptr<wen::RenderPipeline> render_pipeline_;
};