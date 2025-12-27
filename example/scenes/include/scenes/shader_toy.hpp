#pragma once

#include "scenes.hpp"

struct ShaderToyInput {
    ShaderToyInput(wen::Interface& interface);

    struct ShadertoyInputUniform {
        alignas(16) glm::vec3 iResolution;
        alignas(4) float iTime;
        alignas(4) float iTimeDelta;
        alignas(4) float iFrameRate;
        alignas(4) float iFrame;
        alignas(16) glm::vec4 iMouse;
        alignas(16) glm::vec4 iDate;
    };

    ShadertoyInputUniform* data;
    std::shared_ptr<wen::UniformBuffer> uniform_buffer;
};

class ShaderToy : public Scene {
public:
    ShaderToy(std::shared_ptr<wen::Interface> interface) : Scene(interface) {}

    void initialize() override;
    void update(float ts, float w, float h) override;
    void render(float w, float h) override;
    void imgui() override;
    void destroy() override;

private:
    float time_ = 0.0f;
    std::unique_ptr<ShaderToyInput> input_;
    std::shared_ptr<wen::ShaderProgram> shader_program_;
    std::shared_ptr<wen::VertexBuffer> vertex_buffer_;
    std::shared_ptr<wen::IndexBuffer> index_buffer_;
    std::shared_ptr<wen::PushConstants> push_constants_;
    std::shared_ptr<wen::RenderPipeline> render_pipeline_;
};