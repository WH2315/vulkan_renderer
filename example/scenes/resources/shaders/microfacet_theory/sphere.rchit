#version 460

#extension GL_GOOGLE_include_directive : require
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : require

#include "hit_color.glsl"

// sphere data buffer
layout(binding = 3, scalar) buffer SphereDataBuffer {
    Sphere spheres[];
} sphere_data_buffer;

// custom sphere material data buffer
layout(binding = 4, scalar) buffer MaterialDataBuffer {
    Material materials[];
} sphere_material_data_buffer;

void main() {
    Sphere sphere = sphere_data_buffer.spheres[gl_PrimitiveID];
    Material material = sphere_material_data_buffer.materials[gl_PrimitiveID];

    vec3 center = (gl_ObjectToWorldEXT * vec4(sphere.center, 1.0)).xyz;
    vec3 position = gl_WorldRayOriginEXT + gl_HitTEXT * gl_WorldRayDirectionEXT;
    vec3 normal = normalize(position - center);

    if (sphere.radius > 100) {
        float r = sqrt(rnd(ray.state));
        float theta = 2 * PI * rnd(ray.state);
        vec3 L_local = vec3(r * cos(theta), sqrt(1 - r * r), r * sin(theta));

        vec3 up = abs(normal.y) < 0.99999 ? vec3(0, 1, 0) : vec3(0, 0, 1);
        vec3 X = normalize(cross(up, normal));
        vec3 Z = normalize(cross(X, normal));

        if (mod(floor(position.x * 8), 8) == 0 || mod(floor(position.z * 8), 8) == 0) {
            ray.albedo *= 0.05;
        }
        ray.albedo *= material.albedo;
        ray.origin = position;
        ray.direction = normalize(L_local.x * X + L_local.y * normal + L_local.z * Z);
    } else {
        computeHitColor(position, normal, material); 
    }
}