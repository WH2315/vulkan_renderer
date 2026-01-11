#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#include "ray.glsl"

layout(location = 0) rayPayloadInEXT Ray ray;

void main() {
    ray.end = true;
}