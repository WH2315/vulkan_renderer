#include "hittable/sphere.hpp"
#include "tools/onb.hpp"
#include <glm/ext/scalar_constants.hpp>

Sphere::Sphere(const glm::vec3& position, float radius, const std::shared_ptr<Material>& material) {
    moving = false;
    this->position = position;
    this->radius = radius;
    this->material = material;
    aabb = AABB(position - glm::vec3(radius), position + glm::vec3(radius));
}

Sphere::Sphere(const glm::vec3& src, const glm::vec3& dst, float radius, const std::shared_ptr<Material>& material) {
    moving = true;
    this->position = src;
    this->direction = dst - src;
    this->radius = radius;
    this->material = material;
    glm::vec3 rvec = glm::vec3(radius);
    AABB box1(src - rvec, src + rvec);
    AABB box2(dst - rvec, dst + rvec);
    aabb = AABB(box1, box2);
}

bool Sphere::hit(const Ray& ray, Interval t, HitRecord& hit_record) const {
    glm::vec3 center = moving ? position + direction * ray.time : position;
    glm::vec3 origin = center - ray.origin;

    float a = glm::dot(ray.direction, ray.direction);
    float h = glm::dot(ray.direction, origin);
    float c = glm::dot(origin, origin) - radius * radius;

    float discriminant = h * h - a * c;
    if (discriminant < 0.0f) {
        return false;
    }

    float sqtrd = glm::sqrt(discriminant);
    float root = (h - sqtrd) / a;
    if (!t.inside(root)) {
        root = (h + sqtrd) / a;
        if (!t.inside(root)) {
            return false;
        }
    }

    hit_record.t = root;
    hit_record.point = ray.hitPoint(root);
    glm::vec3 normal = (hit_record.point - center) / radius;
    hit_record.setNormal(ray, normal);
    hit_record.material = material;
    uv(normal, hit_record.u, hit_record.v);
    return true;
}

float Sphere::pdfValue(const glm::vec3& origin, const glm::vec3& direction) const {
    HitRecord hit_record;
    if (!hit(Ray(origin, direction), Interval(0.001f, infinity), hit_record)) {
        return 0.0f;
    }
    float cos_theta_max = glm::sqrt(1.0f - radius * radius / glm::dot(position - origin, position - origin));
    float solid_angle = 2.0f * glm::pi<float>() * (1.0f - cos_theta_max);
    return 1.0f / solid_angle;
} 

glm::vec3 Sphere::random(const glm::vec3& origin) const {
    glm::vec3 direction = position - origin;
    float distance2 = glm::dot(direction, direction);
    ONB uvw;
    uvw.build(direction);
    return uvw.local(random_(radius, distance2));
}

void Sphere::uv(const glm::vec3& point, float& u, float& v) {
    float theta = acos(-point.y);
    float phi = atan2(-point.z, point.x) + glm::pi<float>();

    u = phi / (2.0f * glm::pi<float>());
    v = theta / glm::pi<float>();
}

glm::vec3 Sphere::random_(float radius, float distance2) {
    auto r1 = Random::Float();
    auto r2 = Random::Float();
    auto z = 1.0f + r2 * (glm::sqrt(1.0f - radius * radius / distance2) - 1.0f);
    auto phi = 2.0f * glm::pi<float>() * r1;
    auto x = glm::cos(phi) * glm::sqrt(1.0f - z * z);
    auto y = glm::sin(phi) * glm::sqrt(1.0f - z * z);
    return glm::vec3(x, y, z);
}