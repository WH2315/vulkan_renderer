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

const float PI = 3.1415926535;

void main() {
    Sphere sphere = sphere_data_buffer.spheres[gl_PrimitiveID];
    Material material = sphere_material_data_buffer.materials[gl_PrimitiveID];

    // 把球心从物体局部空间变换到世界空间
    vec3 center = (gl_ObjectToWorldEXT * vec4(sphere.center, 1.0)).xyz;
    // o + td，得到世界空间的命中位置 
    vec3 position = gl_WorldRayOriginEXT + gl_HitTEXT * gl_WorldRayDirectionEXT;
    // 球面法线
    vec3 normal = normalize(position - center);

    if (sphere.radius > 100) {
        // 余弦重要性采样
        float r = sqrt(rnd(ray.state)); // cos_theta
        float phi = 2 * PI * rnd(ray.state);
        // 局部空间的采样方向
        vec3 L_local = vec3(r * cos(phi), sqrt(1 - r * r), r * sin(phi));

        // 构建以命中点法线为Y轴的局部空间坐标系
        // 用来把局部采样方向变换到世界空间
        vec3 Y = normal;
        vec3 up = abs(Y.y) < 0.999 ? vec3(0, 1, 0) : vec3(0, 0, 1);
        vec3 X = normalize(cross(up, Y));
        vec3 Z = normalize(cross(X, Y));

        if (mod(floor(position.x * 8), 8) == 0 || mod(floor(position.z * 8), 8) == 0) {
            ray.albedo *= 0.05;
        }
        ray.albedo *= material.albedo;
        ray.origin = position;
        // 将局部采样方向变换到世界空间，确保采样围绕球面法线分布
        ray.direction = normalize(L_local.x * X + L_local.y * Y + L_local.z * Z);
    } else {
        computeHitColor(position, normal, material); 
    }
}