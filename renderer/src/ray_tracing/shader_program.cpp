#include "ray_tracing/shader_program.hpp"

namespace wen {

RayTracingShaderProgram::~RayTracingShaderProgram() {
    if (raygen_shader_) {
        raygen_shader_.reset();
    }
    for (auto& shader : miss_shaders_) {
        shader.reset();
    }
    miss_shaders_.clear();
    for (auto& hit_group : hit_groups_) {
        hit_group.closest_hit_shader.reset();
        if (hit_group.intersection_shader.has_value()) {
            hit_group.intersection_shader->reset();
        }
    }
    hit_groups_.clear();
    hit_shader_count_ = 0;
}

void RayTracingShaderProgram::setRaygenShader(const std::shared_ptr<Shader>& shader) {
    raygen_shader_ = shader;
}

void RayTracingShaderProgram::setMissShader(const std::shared_ptr<Shader>& shader) {
    miss_shaders_.emplace_back(shader);
}

void RayTracingShaderProgram::setHitGroup(const HitGroup& hit_group) {
    hit_groups_.emplace_back(hit_group);
    hit_shader_count_++;
    if (hit_group.intersection_shader.has_value()) {
        hit_shader_count_++;
    }
}

} // namespace wen