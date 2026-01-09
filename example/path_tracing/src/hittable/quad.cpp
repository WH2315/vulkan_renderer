#include "hittable/quad.hpp"

Quad::Quad(const glm::vec3& Q, const glm::vec3& u, const glm::vec3& v, const std::shared_ptr<Material>& material) {
    this->Q = Q;
    this->u = u;
    this->v = v;
    this->material = material;
    auto n = glm::cross(u, v);
    normal = glm::normalize(n);
    D = glm::dot(normal, Q);
    w = n / glm::dot(n, n);
    area = glm::length(n);
    aabb = AABB(Q, Q + u + v);
}

bool Quad::hit(const Ray& ray, Interval t, HitRecord& hit_record) const {
    auto denominator = glm::dot(normal, ray.direction);
    if (glm::abs(denominator) < 1e-8) {
        return false;
    }

    auto _t = (D - glm::dot(normal, ray.origin)) / denominator;
    if (!t.contains(_t)) {
        return false;
    }

    auto point = ray.hitPoint(_t);
    auto _w = point - Q;
    auto alpha = glm::dot(w, glm::cross(_w, v));
    auto beta = glm::dot(w, glm::cross(u, _w));
    if (!isInterior(alpha, beta, hit_record)) {
        return false;
    }

    hit_record.t = _t;
    hit_record.point = point;
    hit_record.setNormal(ray, normal);
    hit_record.material = material;

    return true;
}

bool Quad::isInterior(float a, float b, HitRecord& hit_record) {
    if ((a < 0) || (1 < a) || (b < 0) || (1 < b)) {
        return false;
    }

    hit_record.u = a;
    hit_record.v = b;
    return true;
}

float Quad::pdf(const glm::vec3& origin, const glm::vec3& direction) const {
    HitRecord hit_record;
    if (!hit(Ray(origin, direction), Interval(0.001f, infinity), hit_record)) {
        return 0.0f;
    }
    float distance2 = hit_record.t * hit_record.t * glm::dot(direction, direction);
    float cos_theta = glm::abs(glm::dot(direction, hit_record.normal)) / glm::length(direction);
    return distance2 / (cos_theta * area);
}

glm::vec3 Quad::random(const glm::vec3& origin) const {
    return Q + u * Random::Float() + v * Random::Float() - origin;
}