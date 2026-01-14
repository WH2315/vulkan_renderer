#include "resources/pdf.hpp"
#include "tools/random.hpp"
#include <glm/ext/scalar_constants.hpp>

// CosinePDF
CosinePDF::CosinePDF(const glm::vec3& w) {
    uvw_.build(w);
}

float CosinePDF::pdf(const glm::vec3& direction) const {
    float cos_theta = glm::dot(glm::normalize(direction), uvw_.w());
    return glm::max(0.0f, cos_theta / glm::pi<float>());
}

glm::vec3 CosinePDF::generate() const {
    return uvw_.local(CosineDirection());
}

glm::vec3 CosinePDF::CosineDirection() {
    auto r = glm::sqrt(Random::Float());
    auto phi = 2.0f * glm::pi<float>() * Random::Float();
    auto x = r * glm::cos(phi);
    auto y = r * glm::sin(phi);
    auto z = glm::sqrt(1.0f - r * r);
    return glm::vec3(x, y, z);
}

// SpherePDF
float SpherePDF::pdf(const glm::vec3& direction) const {
    return 1.0f / (4.0f * glm::pi<float>());
}

glm::vec3 SpherePDF::generate() const {
    return Random::UnitSphere();
}

// HittablePDF
HittablePDF::HittablePDF(const std::shared_ptr<Hittable>& hittable, const glm::vec3& origin)
    : hittable_(hittable), origin_(origin) {}

float HittablePDF::pdf(const glm::vec3& direction) const {
    return hittable_->pdf(origin_, direction);
}

glm::vec3 HittablePDF::generate() const {
    return hittable_->random(origin_);
}

// MixturePDF
MixturePDF::MixturePDF(std::shared_ptr<PDF> p0, std::shared_ptr<PDF> p1) {
    p_[0] = p0;
    p_[1] = p1;
}

float MixturePDF::pdf(const glm::vec3& direction) const {
    return 0.5f * p_[0]->pdf(direction) + 0.5f * p_[1]->pdf(direction);
}

glm::vec3 MixturePDF::generate() const {
    if (Random::Float() < 0.5f) {
        return p_[0]->generate();
    } else {
        return p_[1]->generate();
    }
}