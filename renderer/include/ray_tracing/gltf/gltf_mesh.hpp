#pragma once

#include "ray_tracing/gltf/gltf_primitive.hpp"

namespace wen {

struct GLTFMesh {
    std::vector<std::shared_ptr<GLTFPrimitive>> primitives = {};

    GLTFMesh(GLTFScene& scene, const tinygltf::Model& model, const tinygltf::Mesh& mesh,
             const std::vector<std::string>& attrs) {
        WEN_DEBUG("GLTF: load mesh: {}", mesh.name)
        for (auto& primitive : mesh.primitives) {
            if (primitive.mode != TINYGLTF_MODE_TRIANGLES) {
                WEN_WARN("GLTF: only triangle mode is supported, skipping primitive")
                continue;
            }
            if (primitive.indices <= -1) {
                WEN_WARN("GLTF: primitive has no indices, skipping primitive")
                continue;
            }
            primitives.push_back(
                std::make_shared<GLTFPrimitive>(scene, model, primitive, attrs));
        }
    }

    ~GLTFMesh() { primitives.clear(); }
};

} // namespace wen