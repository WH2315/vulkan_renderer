#version 460

#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : require
#extension GL_EXT_buffer_reference : require
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : require

#include "ray.glsl"
#include "random.glsl"
#include "ray_tracing.glsl"

// instance address buffer
layout(binding = 3, scalar) readonly buffer InstanceAddressBuffer {
    InstanceAddress addresses[];
} instance_address_buffer;

// GLTF: primitive data buffer
layout(binding = 4, scalar) readonly buffer PrimitiveDataBuffer {
    GLTFPrimitiveData primitives[];
} primitive_data_buffer;

// material buffer
layout(binding = 5, scalar) readonly buffer MaterialBuffer {
    GLTFMaterial materials[];
} material_buffer;

// NORMAL
layout(binding = 6, scalar) readonly buffer NormalBuffer {
    vec3 normals[];
} normal_buffer;

// 物体的位置
layout(buffer_reference, scalar) buffer Vertices {
    vec3 vertices[];
};

// 三角形的索引
layout(buffer_reference, scalar) buffer Indices {
    Index indices[];
};

layout(push_constant) uniform PushConstant {
    float time;
    int sample_count;
    int frame_index;
    float roughness;
    float metallic;
    vec3 F0;
    float intensity;
    float prob;
} constant;

layout(location = 0) rayPayloadInEXT Ray ray;
hitAttributeEXT vec3 attribs;

const float PI = 3.1415926535;

// 反射强度函数
vec3 F(vec3 F0, float cos_theta) {
    return F0 + (1 - F0) * pow(clamp(1 - cos_theta, 0.0, 1.0), 5.0);
}

// 法线分布函数
float NDF_GGX(vec3 N, vec3 M, float roughness2) {
    float NoM = dot(N, M);
    if (NoM <= 0) {
        return 0;
    }
    // 确保不会0/0
    if (roughness2 < 1e-3) {
        return 1;
    }
    float n = 1 + NoM * NoM * (roughness2 - 1);
    return clamp(roughness2 / (PI * n * n + 0.001), 0, 1);
}

// 几何遮挡函数
// G/4(NoV)(NoL)
float G2(float NoL, float NoV, float roughness2) {
    float lambda_in = NoV * sqrt(roughness2 + NoL * sqrt(NoL - roughness2 * NoL));
    float lambda_out = NoL * sqrt(roughness2 + NoV * sqrt(NoV - roughness2 * NoV));
    return 0.5 / (lambda_in + lambda_out + 0.001);
}

vec3 BRDF(vec3 direction, vec3 albedo, vec3 N, vec3 V, vec3 F0, float roughness2, float metallic) {
    vec3 H = normalize(V + direction);      // 半程向量
    float NoL = max(dot(N, direction), 0);  // 入射角
    float NoV = max(dot(N, V), 0);          // 视角
    float LoH = max(dot(direction, H), 0);

    vec3 diffuse_brdf = albedo * (1 - metallic) / PI;
    vec3 spec_brdf = F(F0, LoH) * G2(NoL, NoV, roughness2) * NDF_GGX(N, H, roughness2);

    return (diffuse_brdf + spec_brdf) * NoL;   
}

void main() {
    InstanceAddress instance_address = instance_address_buffer.addresses[gl_InstanceCustomIndexEXT];
    GLTFPrimitiveData primitive_data = primitive_data_buffer.primitives[gl_InstanceCustomIndexEXT];

    Vertices vertices = Vertices(instance_address.vertex_buffer_address);
    Indices indices = Indices(instance_address.index_buffer_address);

    GLTFMaterial material = material_buffer.materials[primitive_data.material_index];
    Index idxs = indices.indices[primitive_data.first_index / 3 + gl_PrimitiveID];
    idxs.i0 += primitive_data.first_vertex;
    idxs.i1 += primitive_data.first_vertex;
    idxs.i2 += primitive_data.first_vertex;
    const vec3 v0 = vertices.vertices[idxs.i0];
    const vec3 v1 = vertices.vertices[idxs.i1];
    const vec3 v2 = vertices.vertices[idxs.i2];
    const vec3 n0 = normal_buffer.normals[idxs.i0];
    const vec3 n1 = normal_buffer.normals[idxs.i1];
    const vec3 n2 = normal_buffer.normals[idxs.i2];

    vec3 barycentrics = vec3(1.0 - attribs.x - attribs.y, attribs.x, attribs.y);

    vec3 position = v0 * barycentrics.x + v1 * barycentrics.y + v2 * barycentrics.z;
    position = (gl_ObjectToWorldEXT * vec4(position, 1.0)).xyz;
    vec3 normal = normalize(n0 * barycentrics.x + n1 * barycentrics.y + n2 * barycentrics.z);
    normal = normalize((gl_ObjectToWorldEXT * vec4(normal, 0.0)).xyz);

    float roughness = constant.roughness;
    float roughness2 = roughness * roughness;

    // cosine重要性采样，得到局部空间的采样方向
    float r = sqrt(rnd(ray.state));
    float phi = 2 * PI * rnd(ray.state);
    vec3 cosine_direction_local = vec3(r * cos(phi), sqrt(1 - r * r), r * sin(phi));
    // 将局部空间转为世界空间
    vec3 y_axis = normal;
    vec3 up = abs(y_axis.y) < 0.999 ? vec3(0, 1, 0) : vec3(0, 0, 1);
    vec3 x_axis = normalize(cross(up, y_axis));
    vec3 z_axis = normalize(cross(x_axis, y_axis));
    vec3 cosine_direction = normalize(
        cosine_direction_local.x * x_axis +
        cosine_direction_local.y * y_axis +
        cosine_direction_local.z * z_axis
    );
    // cosine重要性采样的pdf由出射方向与法线夹角的cos得出
    // 夹角的cosine值在出射半球上的积分为PI
    float cosine_pdf = dot(cosine_direction, normal) / PI;

    // 完美的漫反射，余弦重要性采样可以很好的降低方差
    // 但对于roughness不为1的物体，也就是光滑的物体，越光滑，cosine重要性采样的效果越差
    // 所以同样使用完美镜面反射采样一个出射方向
    vec3 reflect_direction = reflect(ray.direction, normal);
    float reflect_pdf = 1;

    // 混合两种采样方式
    ray.direction = normalize(mix(
        reflect_direction,
        cosine_direction,
        roughness
    ));
    float pdf = mix(
        reflect_pdf,
        cosine_pdf,
        roughness
    );

    // 俄罗斯轮盘赌，每条光线有prob的概率继续传播
    ray.end = rnd(ray.state) > constant.prob;

    ray.color += ray.albedo * material.emissive_factor * constant.intensity;

    vec3 brdf_cosine = BRDF(
        ray.direction,
        material.base_color_factor.rgb,
        normal,
        normalize(ray.origin - position),
        constant.F0,
        roughness2,
        constant.metallic
    );

    // 因为只有prob的光线,所以要除以prob
    ray.albedo *= brdf_cosine / pdf / constant.prob;

    ray.origin = position;
}