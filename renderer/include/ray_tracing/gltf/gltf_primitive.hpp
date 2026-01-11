#pragma once

#include "resources/model.hpp"
#include <tiny_gltf.h>
#include <glm/glm.hpp>

namespace wen {

class GLTFScene;

class GLTFPrimitive : public Model {
    friend class RayTracingInstance;

public:
    struct GLTFPrimitiveData {
        uint32_t first_index = 0;
        uint32_t first_vertex = 0;
        uint32_t material_index = 0;
    };

    GLTFPrimitive(GLTFScene& scene, const tinygltf::Model& model,
                  const tinygltf::Primitive& primitive,
                  const std::vector<std::string>& attrs);
    ~GLTFPrimitive() override = default;

    uint32_t vertex_count = 0;
    uint32_t index_count = 0;

    ModelType getType() const override { return ModelType::eGLTFPrimitive; }

    auto& getData() { return data_; }

private:
    GLTFScene& scene_;
    GLTFPrimitiveData data_;
    glm::vec3 min_;
    glm::vec3 max_;
};

} // namespace wen