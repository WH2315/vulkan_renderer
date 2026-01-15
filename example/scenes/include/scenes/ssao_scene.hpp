#pragma once

#include <array>

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
    std::shared_ptr<wen::NormalModel> model_;
    std::shared_ptr<wen::VertexBuffer> vertex_buffer_;
    std::shared_ptr<wen::IndexBuffer> index_buffer_;

    std::shared_ptr<wen::Sampler> sampler_;
    std::shared_ptr<wen::UniformBuffer> ssao_samples_uniform_buffer_;
    std::shared_ptr<wen::DataTexture> ssao_random_vectors_texture_;

    std::shared_ptr<wen::DescriptorSet> prepare_ds_;
    std::shared_ptr<wen::DescriptorSet> ssao_ds_;
    std::shared_ptr<wen::DescriptorSet> main_ds_;

    std::shared_ptr<wen::GraphicsShaderProgram> prepare_sp_;
    std::shared_ptr<wen::GraphicsShaderProgram> ssao_sp_;
    std::shared_ptr<wen::GraphicsShaderProgram> main_sp_;
    std::shared_ptr<wen::GraphicsRenderPipeline> prepare_rp_;
    std::shared_ptr<wen::GraphicsRenderPipeline> ssao_rp_;
    std::shared_ptr<wen::GraphicsRenderPipeline> main_rp_;

    std::shared_ptr<wen::PushConstants> ssao_pcs_;
    std::shared_ptr<wen::PushConstants> main_pcs_;

    std::array<vk::ImageView, 3> last_input_views_{};
};