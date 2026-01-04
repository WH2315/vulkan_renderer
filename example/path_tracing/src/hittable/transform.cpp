#include "hittable/transform.hpp"

Translate::Translate(const std::shared_ptr<Hittable>& hittable, const glm::vec3& displacement)
    : hittable_(hittable), offset_(displacement) {
    aabb = hittable->aabb + offset_;
}

bool Translate::hit(const Ray& ray, Interval t, HitRecord& hit_record) const {
    Ray ray_offset(ray.origin - offset_, ray.direction, ray.time);

    if (!hittable_->hit(ray_offset, t, hit_record)) {
        return false;
    }

    hit_record.point += offset_;
    return true;
}

Rotate::Rotate(const std::shared_ptr<Hittable>& hittable, float angle) : hittable_(hittable) {
    auto radians = glm::radians(angle);
    sin_theta_ = glm::sin(radians);
    cos_theta_ = glm::cos(radians);
    aabb = hittable->aabb;

    glm::vec3 min(infinity, infinity, infinity);
    glm::vec3 max(-infinity, -infinity, -infinity);

    for (int i = 0; i < 2; i++) {
        for (int j = 0; j < 2; j++) {
            for (int k = 0; k < 2; k++) {
                auto x = i * aabb.x.max + (1 - i) * aabb.x.min;
                auto y = j * aabb.y.max + (1 - j) * aabb.y.min;
                auto z = k * aabb.z.max + (1 - k) * aabb.z.min;

                auto newx = cos_theta_ * x + sin_theta_ * z;
                auto newz = -sin_theta_ * x + cos_theta_ * z;

                glm::vec3 tester(newx, y, newz);

                for (int c = 0; c < 3; c++) {
                    min[c] = glm::min(min[c], tester[c]);
                    max[c] = glm::max(max[c], tester[c]);
                }
            }
        }
    }

    aabb = AABB(min, max);
}

bool Rotate::hit(const Ray& ray, Interval t, HitRecord& hit_record) const {
    auto origin = ray.origin;
    auto direction = ray.direction;

    origin[0] = cos_theta_ * ray.origin[0] - sin_theta_ * ray.origin[2];
    origin[2] = sin_theta_ * ray.origin[0] + cos_theta_ * ray.origin[2];
    direction[0] = cos_theta_ * ray.direction[0] - sin_theta_ * ray.direction[2];
    direction[2] = sin_theta_ * ray.direction[0] + cos_theta_ * ray.direction[2];

    Ray ray_rotate(origin, direction, ray.time);

    if (!hittable_->hit(ray_rotate, t, hit_record)) {
        return false;
    }

    auto point = hit_record.point;
    point[0] = cos_theta_ * hit_record.point[0] + sin_theta_ * hit_record.point[2];
    point[2] = -sin_theta_ * hit_record.point[0] + cos_theta_ * hit_record.point[2];

    auto normal = hit_record.normal;
    normal[0] = cos_theta_ * hit_record.normal[0] + sin_theta_ * hit_record.normal[2];
    normal[2] = -sin_theta_ * hit_record.normal[0] + cos_theta_ * hit_record.normal[2];
    hit_record.point = point;
    hit_record.normal = normal;

    return true;
}