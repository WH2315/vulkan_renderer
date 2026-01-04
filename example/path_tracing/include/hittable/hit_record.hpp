#pragma once

#include "resources/ray.hpp"
#include <memory>

class Material;
class HitRecord {
public:
    float t;
    glm::vec3 point;
    bool outside;
    glm::vec3 normal;
    std::shared_ptr<Material> material;
    float u, v;

    void setNormal(const Ray& ray, const glm::vec3& outward) {
        outside = glm::dot(ray.direction, outward) < 0;
        normal = outside ? outward : -outward;
    }
};