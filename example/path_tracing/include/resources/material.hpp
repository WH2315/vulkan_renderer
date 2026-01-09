#pragma once

#include "resources/ray.hpp"
#include "hittable/hit_record.hpp"
#include "tools/random.hpp"
#include "resources/textures.hpp"
#include "resources/pdf.hpp"
#include <glm/ext/scalar_constants.hpp>

class ScatterRecord {
public:
    glm::vec3 attenuation;
    std::shared_ptr<PDF> pdf = nullptr;
    Ray ray_out;
};

class Material {
public:
    virtual ~Material() = default;
    virtual bool scatter(const Ray& ray_in, const HitRecord& hit_record, ScatterRecord& scatter_record) const { return false; }
    virtual glm::vec3 emitted(const HitRecord& hit_record) const { return glm::vec3(0.0f); }
    virtual float brdf(const HitRecord& hit_record, const Ray& ray_out) const { return 0.0f; }
};

// 朗伯材质
class Lambertian : public Material {
public:
    explicit Lambertian(const glm::vec3& albedo) : albedo(std::make_shared<SolidColor>(albedo)) {}
    explicit Lambertian(const std::shared_ptr<Texture>& albedo) : albedo(albedo) {}

    bool scatter(const Ray& ray_in, const HitRecord& hit_record, ScatterRecord& scatter_record) const override {
        scatter_record.attenuation = albedo->value(hit_record.u, hit_record.v, hit_record.point);
        scatter_record.pdf = std::make_shared<CosinePDF>(hit_record.normal);
        return true; 
    }

    float brdf(const HitRecord& hit_record, const Ray& ray_out) const override {
        float cos_theta = glm::dot(hit_record.normal, ray_out.direction);
        return glm::max(0.0f, cos_theta / glm::pi<float>());
    }

    std::shared_ptr<Texture> albedo;
};

// 金属材质
class Metal : public Material {
public:
    Metal(const glm::vec3& albedo, float roughness) : albedo(albedo), roughness(roughness) {}

    bool scatter(const Ray& ray_in, const HitRecord& hit_record, ScatterRecord& scatter_record) const override {
        scatter_record.attenuation = albedo;

        glm::vec3 reflected = glm::reflect(glm::normalize(ray_in.direction), hit_record.normal);
        auto direction = reflected + roughness * Random::UnitSphere();
        scatter_record.ray_out = Ray(hit_record.point, direction, ray_in.time);
        return true;
    }

    glm::vec3 albedo;
    float roughness;
};

// 电介质材质
class Dielectric : public Material {
public:
    explicit Dielectric(float ir) : ir(ir) {}

    bool scatter(const Ray& ray_in, const HitRecord& hit_record, ScatterRecord& scatter_record) const override {
        scatter_record.attenuation = glm::vec3(1.0f);

        float refraction_ratio = hit_record.outside ? (1.0f / ir) : ir;
        glm::vec3 unit_direction = glm::normalize(ray_in.direction);
        float cos_theta = glm::min(glm::dot(-unit_direction, hit_record.normal), 1.0f);
        float sin_theta = glm::sqrt(1.0f - cos_theta * cos_theta);

        glm::vec3 direction;
        if (refraction_ratio * sin_theta > 1.0f || reflectance(cos_theta, refraction_ratio) > Random::Float()) {
            direction = glm::reflect(unit_direction, hit_record.normal);
        } else {
            direction = glm::refract(unit_direction, hit_record.normal, refraction_ratio);
        }

        scatter_record.ray_out = Ray(hit_record.point, direction, ray_in.time);
        return true;
    }

    float ir;
    static double reflectance(float cos_theta, float ir) {
        auto r0 = (1 - ir) / (1 + ir);
        r0 = r0 * r0;
        return r0 + (1 - r0) * pow((1 - cos_theta), 5);
    }
};

// 散射光材质
class DiffuseLight : public Material {
public:
    DiffuseLight(const glm::vec3& emit) : emit(std::make_shared<SolidColor>(emit)) {}
    DiffuseLight(const std::shared_ptr<Texture>& emit) : emit(emit) {}

    glm::vec3 emitted(const HitRecord& hit_record) const override {
        if (!hit_record.outside) {
            return glm::vec3(0.0f);
        }
        return emit->value(hit_record.u, hit_record.v, hit_record.point);
    }

    std::shared_ptr<Texture> emit;
};

// 各向同性材质
class Isotropic : public Material {
public:
    Isotropic(const glm::vec3& albedo) : albedo(std::make_shared<SolidColor>(albedo)) {}
    Isotropic(const std::shared_ptr<Texture>& albedo) : albedo(albedo) {}

    bool scatter(const Ray& ray_in, const HitRecord& hit_record, ScatterRecord& scatter_record) const override {
        scatter_record.attenuation = albedo->value(hit_record.u, hit_record.v, hit_record.point);
        scatter_record.pdf = std::make_shared<SpherePDF>();
        return true;
    }

    float brdf(const HitRecord& hit_record, const Ray& ray_out) const override {
        return 1.0f / (4.0f * glm::pi<float>());
    }

    std::shared_ptr<Texture> albedo;
};