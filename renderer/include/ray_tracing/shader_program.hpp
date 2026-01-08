#pragma once

#include "resources/shader.hpp"
#include <memory>

namespace wen {

class RayTracingShaderProgram {
    friend class RayTracingRenderPipeline;

public:
    struct HitGroup {
        std::shared_ptr<Shader> closest_hit_shader;
        std::optional<std::shared_ptr<Shader>> intersection_shader;
    };

    RayTracingShaderProgram() : hit_shader_count_(0) {}

    ~RayTracingShaderProgram();

    void setRaygenShader(const std::shared_ptr<Shader>& shader);
    void setMissShader(const std::shared_ptr<Shader>& shader);
    void setHitGroup(const HitGroup& hit_group);

private:
    std::shared_ptr<Shader> raygen_shader_;
    std::vector<std::shared_ptr<Shader>> miss_shaders_;
    std::vector<HitGroup> hit_groups_;
    uint32_t hit_shader_count_ = 0;
};

} // namespace wen