#pragma once

#include <random>
#include <glm/glm.hpp>

class Random {
public:
    static void Init() {
        random_engine.seed(std::random_device()());
    }

    static uint32_t UInt() {
        return distribution(random_engine);
    }

    // [min, max]
    static uint32_t UInt(uint32_t min, uint32_t max) {
        return min + (distribution(random_engine) % (max - min + 1));
    }

    // (0, 1)
    static float Float() {
        return (float)distribution(random_engine) / (float)std::numeric_limits<uint32_t>::max();
    }

    // [min, max)
    static float Float(float min, float max) {
        return min + Float() * (max - min);
    }

    static glm::vec3 Vec3() {
        return glm::vec3(Float(), Float(), Float());
    }

    static glm::vec3 Vec3(float min, float max) {
        return glm::vec3(Float() * (max - min) + min, Float() * (max - min) + min, Float() * (max - min) + min);
    }

    static glm::vec3 UnitSphere() {
        while (true) {
            auto unit = glm::vec3(2.0f * Float() - 1.0f, 2.0f * Float() - 1.0f, 2.0f * Float() - 1.0f);
            if (glm::dot(unit, unit) < 1) {
                return glm::normalize(unit);
            }
        }
    }

    static thread_local std::mt19937 random_engine;
    static std::uniform_int_distribution<std::mt19937::result_type> distribution;
};