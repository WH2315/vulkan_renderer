#version 450

layout(location = 0) in vec3 positions;
layout(location = 1) in vec3 colors;
layout(location = 2) in vec2 uvs;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec2 frag_uv;

layout(binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 project;
} ubo;

layout(push_constant) uniform PushConstants {
    float pad;
    vec3 offset;
} pc;

void main() {
    gl_Position = ubo.project * ubo.view * ubo.model * vec4(positions + pc.offset, 1.0);
    frag_color = colors;
    frag_uv = uvs;
}