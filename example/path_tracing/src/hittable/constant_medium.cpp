#include "hittable/constant_medium.hpp"

ConstantMedium::ConstantMedium(const std::shared_ptr<Hittable>& boundary, float density, const std::shared_ptr<Texture>& albedo) {
    boundary_ = boundary;
    neg_inv_density_ = -1.0f / density;
    phase_ = std::make_shared<Isotropic>(albedo);
}

ConstantMedium::ConstantMedium(const std::shared_ptr<Hittable>& boundary, float density, const glm::vec3& albedo) {
    boundary_ = boundary;
    neg_inv_density_ = -1.0f / density;
    phase_ = std::make_shared<Isotropic>(albedo);
}

bool ConstantMedium::hit(const Ray& ray, Interval t, HitRecord& hit_recoed) const {
    HitRecord rec1, rec2;
    if (!boundary_->hit(ray, Interval::universe, rec1)) {
        return false;
    }
    if (!boundary_->hit(ray, Interval(rec1.t + 0.0001f, infinity), rec2)) {
        return false;
    }

    if (rec1.t < t.min) rec1.t = t.min;
    if (rec2.t > t.max) rec2.t = t.max;

    if (rec1.t >= rec2.t) {
        return false;
    }

    if (rec1.t < 0) {
        rec1.t = 0;
    }

    auto ray_length = glm::length(ray.direction);
    auto distance_inside_boundary = (rec2.t - rec1.t) * ray_length;
    auto hit_distance = neg_inv_density_ * log(Random::Float());

    if (hit_distance > distance_inside_boundary) {
        return false;
    }

    hit_recoed.t = rec1.t + hit_distance / ray_length;
    hit_recoed.point = ray.hitPoint(hit_recoed.t);
    hit_recoed.normal = glm::vec3(1.0f, 0.0f, 0.0f);
    hit_recoed.outside = true;
    hit_recoed.material = phase_;

    return true;
}