#pragma once

#include "scenes.hpp"
#include "camera.hpp"

class SSAO : public Scene {
public:
    SSAO(std::shared_ptr<wen::Interface> interface) : Scene(interface) {}

    void initialize() override;
    void update(float ts, float w, float h) override;
    void render(float w, float h) override;
    void imgui() override;
    void destroy() override;

private:
    void refreshInputAttachments();

    std::unique_ptr<Camera> camera_;
    std::shared_ptr<wen::DescriptorSet> main_descriptor_set_;
    std::shared_ptr<wen::DescriptorSet> post_descriptor_set_;
    std::shared_ptr<wen::Sampler> gbuffer_sampler_;
    std::array<vk::ImageView, 3> last_input_views_{};
    std::shared_ptr<wen::GraphicsShaderProgram> main_shader_program_;
    std::shared_ptr<wen::GraphicsShaderProgram> post_shader_program_;
    std::shared_ptr<wen::NormalModel> model_;
    std::shared_ptr<wen::VertexBuffer> vertex_buffer_;
    std::shared_ptr<wen::IndexBuffer> index_buffer_;
    std::shared_ptr<wen::VertexBuffer> offsets_buffer_;
    std::shared_ptr<wen::GraphicsRenderPipeline> main_render_pipeline_;
    std::shared_ptr<wen::GraphicsRenderPipeline> post_render_pipeline_;
};