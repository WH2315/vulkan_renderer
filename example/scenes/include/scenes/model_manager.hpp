#pragma once

#include "scenes.hpp"
#include "camera.hpp"

struct ModelInfo {
    struct InnerInfo {
        glm::vec3 offset;
        float scale;
    };

    std::shared_ptr<wen::NormalModel> model;
    std::shared_ptr<wen::VertexBuffer> vertex_buffer;
    std::vector<InnerInfo> inner_infos;
    std::map<std::string, bool> mesh_visibility;
};

class ModelManager : public Scene {
public:
    ModelManager(std::shared_ptr<wen::Interface> interface) : Scene(interface) {}

    void initialize() override;
    void update(float ts) override;
    void render() override;
    void imgui(VkDescriptorSet image) override;
    void destroy() override;

private:
    std::unique_ptr<Camera> camera_;
    std::shared_ptr<wen::ShaderProgram> shader_program_;
    std::shared_ptr<wen::VertexBuffer> vertex_buffer_;
    std::shared_ptr<wen::IndexBuffer> index_buffer_;
    std::shared_ptr<wen::RenderPipeline> render_pipeline_;
    // 模型对应的文件名和模型信息
    std::map<std::string, ModelInfo> models_;
    // 实例名 (模型文件名 实例索引)
    std::map<std::string, std::pair<std::string, uint32_t>> querys_;
};