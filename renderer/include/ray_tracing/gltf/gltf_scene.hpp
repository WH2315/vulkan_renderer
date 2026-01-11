#pragma once

#include "ray_tracing/gltf/gltf_primitive.hpp"
#include "ray_tracing/gltf/gltf_mesh.hpp"
#include "ray_tracing/gltf/gltf_node.hpp"
#include "resources/specific_texture.hpp"
#include "resources/sampler.hpp"

namespace wen {

struct GLTFMaterial {
    glm::vec4 base_color_factor = glm::vec4(1.0f);
    int base_color_texture = -1;
    glm::vec3 emissive_factor = glm::vec3(0.0f);
    int emissive_texture = -1;
    int normal_texture = -1;
    float metallic_factor = 1.0f;
    float roughness_factor = 1.0f;
    int metallic_roughness_texture = -1;
};

class DescriptorSet;

class GLTFScene {
    friend class GLTFNode;
    friend class GLTFPrimitive;

public:
    GLTFScene(const std::string& filename, const std::vector<std::string>& attrs);
    ~GLTFScene();

    void build(std::function<void(GLTFNode*, std::shared_ptr<GLTFPrimitive>)> func);
    void bindTexturesSamplers(const std::shared_ptr<DescriptorSet>& descriptor_set,
                              uint32_t binding);

    auto getMaterialBuffer() { return material_buffer_; }

    auto getAttrBuffer(const std::string& name) { return attr_buffers_.at(name); }

    uint32_t getTexturesCount() { return textures_.size(); }

    std::vector<glm::vec3> vertices;
    std::vector<uint32_t> indices;
    std::unique_ptr<Buffer> ray_tracing_vertex_buffer;
    std::unique_ptr<Buffer> ray_tracing_index_buffer;

private:
    void loadImages(const tinygltf::Model& model);
    void loadMaterials(const tinygltf::Model& model);
    void loadMeshesAndPrimitives(const tinygltf::Model& model,
                                 const std::vector<std::string>& attrs);
    void loadAttributes();
    void loadNodes(const tinygltf::Model& model);

private:
    std::string filepath_;

    // textures
    std::vector<std::shared_ptr<SpecificTexture>> textures_;
    std::shared_ptr<Sampler> sampler_;

    // materials
    std::vector<GLTFMaterial> materials_;
    std::shared_ptr<StorageBuffer> material_buffer_;

    // meshes
    std::vector<std::shared_ptr<GLTFMesh>> meshes_;

    // attr
    std::map<std::string, std::vector<uint8_t>> attr_datas_;
    std::map<std::string, std::shared_ptr<StorageBuffer>> attr_buffers_;

    // nodes
    std::vector<std::shared_ptr<GLTFNode>> nodes_;
    std::vector<GLTFNode*> nodes_ptr_;
};

} // namespace wen