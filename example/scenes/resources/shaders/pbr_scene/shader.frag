#version 450

layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec3 fragColor;

layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform CameraUniform {
    vec3 position;
    mat4 view;
    mat4 project;
} camera;

layout(binding = 1) uniform MaterialUniform {
    vec3 albedo;
    float metallic;
    float roughness;
    float ao;
} material;

struct PointLight {
    vec3 position;
    vec3 color;
};

layout(binding = 2) uniform LightsUniform {
    vec3 direction;            // 平行光
    vec3 color;                // 平行光颜色
    PointLight pointLights[3]; // 点光源
} lights;


const float PI = 3.1415926535;

// 法线分布函数
float D_GGX_TR(float NoH, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NoH2 = NoH * NoH;

    float nom = a2;
    float denom = NoH2 * (a2 - 1.0) + 1.0;
    denom = PI * denom * denom;

    return nom / denom;
}

// 几何函数
float GeometrySmith(float NoL, float NoV, float roughness) {
    float k = pow(roughness + 1.0, 2.0) / 8.0;
    float ggx1 = NoV / (NoV * (1.0 - k) + k);
    float ggx2 = NoL / (NoL * (1.0 - k) + k);

    return ggx1 * ggx2;
}

// 菲涅尔方程
vec3 fresnelSchlick(vec3 F0, float cosTheta) {
    float ct = clamp(1.0 - cosTheta, 0.0, 1.0);
    return F0 + (1.0 - F0) * pow(ct, 5.0);
}

vec3 computeDirectionLight(vec3 direction, vec3 color, vec3 N, vec3 V, vec3 F0, float roughness) {
    vec3 L = normalize(direction);
    vec3 H = normalize(L + V);     // 半程向量
    float NoL = max(dot(N, L), 0.0); // 入射角
    float NoV = max(dot(N, V), 0.0); // 视角
    if (NoL <= 0.0 || NoV <= 0.0) {
        return vec3(0.0);
    }
    float NoH = max(dot(N, H), 0.0); // 法线和半程向量夹角
    float VoH = max(dot(V, H), 0.0); // 视线和半程向量夹角

    // cook-torrance brdf
    float D = D_GGX_TR(NoH, roughness);
    float G = GeometrySmith(NoL, NoV, roughness);
    vec3 F = fresnelSchlick(F0, VoH);

    vec3 kS = F;
    vec3 kD = vec3(1.0) - kS;
    kD *= 1.0 - material.metallic;

    // 漫反射
    vec3 diffuse_color = kD * material.albedo / PI;
    // 镜面反射
    vec3 specular_color = (D * G * F) / max(4.0 * NoL * NoV, 0.001);

    return (diffuse_color + specular_color) * NoL * color;
}

vec3 computePointLight(vec3 position, vec3 color, vec3 pos, vec3 N, vec3 V, vec3 F0, float roughness) {
    vec3 L = normalize(position - pos);
    float distance = max(length(position - pos), 0.001);
    return computeDirectionLight(L, color, N, V, F0, roughness) / (distance * distance);
}

void main() {
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, material.albedo, material.metallic);

    // 法线(片段法线方向)
    vec3 N = normalize(fragNormal);
    // 视线(片段位置到相机位置的方向)
    vec3 V = normalize(camera.position - fragPosition);

    vec3 color = computeDirectionLight(-lights.direction, lights.color, N, V, F0, material.roughness);
    for (int i = 0; i < 3; i ++) {
        color += computePointLight(lights.pointLights[i].position, lights.pointLights[i].color, fragPosition, N, V, F0, material.roughness);
    }

    // 环境光
    vec3 ambient = vec3(0.03) * material.albedo * material.ao;

    color += ambient;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    outColor = vec4(color, 1.0);
}
