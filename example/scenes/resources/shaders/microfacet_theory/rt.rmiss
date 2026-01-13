#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#include "ray.glsl"

layout(location = 0) rayPayloadInEXT Ray ray;

const vec3 sky = vec3(0.9, 1, 1);
const vec3 sun = vec3(50);
const vec3 sun_dir = normalize(vec3(0.4, 1.0, 0.5));

void main() {
    ray.end = true;
    if (dot(ray.direction, sun_dir) > 0.997) {
        ray.color += ray.albedo * sun;
    } else {
        ray.color += ray.albedo * sky;
    }
}