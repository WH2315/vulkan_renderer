#version 450

layout(binding = 0, input_attachment_index = 0) uniform subpassInput buffers[3];

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform CameraUniform {
    vec3 position;
    mat4 view;
    mat4 project;
} camera;

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
};

layout(binding = 2) uniform LightUniform {
    PointLight point_lights[8];
    uint light_count;
    int display_mode;
} light;

void main() {
    if (subpassLoad(buffers[0]).w == 1) {
        outColor = vec4(1, 1, 1, 0);
        return;
    }

    // debug modes
    if (light.display_mode == 1) {
        // position
        outColor = vec4(subpassLoad(buffers[0]).xyz, 1);
        return;
    } else if (light.display_mode == 2) {
        // z-buffer
        outColor = vec4(vec3(subpassLoad(buffers[0]).w), 1);
        return; 
    } else if (light.display_mode == 3) {
        // normal
        outColor = vec4((subpassLoad(buffers[1]).xyz + 1) / 2, 1);
        return;
    } else if (light.display_mode == 4) {
        // color
        outColor = vec4(subpassLoad(buffers[2]).rgb, 1);
        return;
    }

    vec3 position = subpassLoad(buffers[0]).xyz;
    vec3 inNormal = subpassLoad(buffers[1]).xyz;
    vec3 inColor = subpassLoad(buffers[2]).rgb;

    vec3 cool = vec3(0, 0, 0.55) + inColor / 4;
    vec3 warm = vec3(0.3, 0.3, 0) + inColor / 4;
    vec3 high_light = vec3(2, 2, 2);
    vec3 color = cool / 2;
    for (uint i = 0; i < light.light_count; i ++) {
        vec3 N = normalize(inNormal);
        vec3 V = normalize(camera.position - position);
        vec3 L = normalize(light.point_lights[i].position - position);
        vec3 R = reflect(-L, N);
        float s = clamp(100 * dot(R, V) - 97, 0, 1);
        color += clamp(dot(L, N), 0, 1) * light.point_lights[i].color * mix(warm, high_light, s) * light.point_lights[i].intensity;
    }

    outColor = vec4(color, 1.0);
}