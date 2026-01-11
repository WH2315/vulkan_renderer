#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_GOOGLE_include_directive : require

#include "ray.glsl"

layout(binding = 0, set = 1) uniform Info {
    vec2 window_size;
    vec3 clear_color;
} info;

layout(location = 0) rayPayloadInEXT HitPayload prd;

void main() {
    prd.value = info.clear_color;
}