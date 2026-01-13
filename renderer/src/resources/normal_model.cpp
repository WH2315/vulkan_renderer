#include "resources/normal_model.hpp"
#include "core/log.hpp"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

namespace std {

template <>
struct hash<wen::Vertex> {
    size_t operator()(wen::Vertex const& vertex) const {
        return ((hash<glm::vec3>()(vertex.position) ^
                 (hash<glm::vec3>()(vertex.normal) << 1)) >>
                1) ^
               (hash<glm::vec3>()(vertex.color) << 1);
    }
};

} // namespace std

namespace wen {

Mesh::Mesh() {}

Mesh::~Mesh() {
    indices.clear();
}

NormalModel::NormalModel(const std::string& filename, const std::vector<std::string>& blacklist)
    : vertex_count(0), index_count(0) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err,
                          filename.c_str())) {
        WEN_ERROR("Failed to load model: {0}", filename)
        throw std::runtime_error(warn + err);
    }

    for (const auto& name : blacklist) {
        auto it = shapes.begin();
        while (it != shapes.end()) {
            if (it->name == name) {
                it = shapes.erase(it);
                break;
            }
            ++it;
        }
    }

    uint32_t size = 0;
    for (const auto& shape : shapes) {
        size += shape.mesh.indices.size();
    }
    vertices_.reserve(size);

    std::unordered_map<Vertex, uint32_t> uniqueVertices = {};
    uniqueVertices.reserve(size);

    for (const auto& shape : shapes) {
        std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>();
        for (const auto& index : shape.mesh.indices) {
            Vertex vertex = {};
            vertex.position = {attrib.vertices[3 * index.vertex_index + 0],
                               attrib.vertices[3 * index.vertex_index + 1],
                               attrib.vertices[3 * index.vertex_index + 2]};
            if (index.normal_index < 0) {
                vertex.normal = {0.0f, 0.0f, 0.0f};
            } else {
                vertex.normal = {attrib.normals[3 * index.normal_index + 0],
                                 attrib.normals[3 * index.normal_index + 1],
                                 attrib.normals[3 * index.normal_index + 2]};
            }
            vertex.color = {1.0f, 1.0f, 1.0f};

            if (uniqueVertices.count(vertex) == 0) {
                uniqueVertices.insert(std::make_pair(vertex, vertices_.size()));
                vertices_.push_back(vertex);
            }
            mesh->indices.push_back(uniqueVertices[vertex]);
        }
        index_count += mesh->indices.size();
        meshes_.insert(std::make_pair(shape.name, std::move(mesh)));
    }
    vertex_count = vertices_.size();
}

Offset NormalModel::upload(std::shared_ptr<VertexBuffer> vertex_buffer,
                           std::shared_ptr<IndexBuffer> index_buffer, Offset offset) {
    offset_ = offset;
    auto temp = offset;
    temp.vertex = vertex_buffer->setData(vertices_, temp.vertex);
    for (auto& [name, mesh] : meshes_) {
        mesh->offset.vertex = offset_.vertex;
        mesh->offset.index = temp.index;
        temp.index = index_buffer->setData(mesh->indices, temp.index);
    }
    return temp;
}

NormalModel::~NormalModel() {
    vertices_.clear();
    meshes_.clear();
    ray_tracing_vertex_buffer.reset();
    ray_tracing_index_buffer.reset();
}

} // namespace wen