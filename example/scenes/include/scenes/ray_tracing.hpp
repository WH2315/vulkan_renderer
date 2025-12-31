#pragma once

#include "scenes.hpp"
#include "camera.hpp"

class RayTracing : public Scene {
public:
    RayTracing(std::shared_ptr<wen::Interface> interface)
        : Scene(std::move(interface)) {
        is_enable_ray_tracing = true;
    }
};