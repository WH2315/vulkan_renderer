#pragma once

#include "resources/model.hpp"
#include "resources/vertex_input/vertex_buffer.hpp"
#include "resources/vertex_input/index_buffer.hpp"
#include <glm/glm.hpp>
#include <map>

namespace wen {

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec3 color;

    bool operator==(const Vertex& other) const {
        return position == other.position && normal == other.normal &&
               color == other.color;
    }
};

struct Offset {
    uint32_t vertex;
    uint32_t index;
};

class Mesh final {
public:
    Mesh();
    ~Mesh();

    Offset offset;
    std::vector<uint32_t> indices;
};

class NormalModel : public Model {
public:
    NormalModel(const std::string& filename, const std::vector<std::string>& blacklist = {});
    ~NormalModel() override;

    uint32_t vertex_count;
    uint32_t index_count;

    auto vertices() const { return vertices_; }

    auto meshes() const { return meshes_; }

    auto offset() const { return offset_; }

    Offset upload(std::shared_ptr<VertexBuffer> vertex_buffer,
                  std::shared_ptr<IndexBuffer> index_buffer, Offset offset = {0, 0});

    ModelType getType() const override { return ModelType::eNormalModel; }

    std::unique_ptr<Buffer> ray_tracing_vertex_buffer;
    std::unique_ptr<Buffer> ray_tracing_index_buffer;

private:
    std::vector<Vertex> vertices_;
    std::map<std::string, std::shared_ptr<Mesh>> meshes_;
    Offset offset_;
};

} // namespace wen
