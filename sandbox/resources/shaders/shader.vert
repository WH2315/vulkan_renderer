#version 450

layout(location = 0) in vec3 positions;
layout(location = 1) in vec3 colors;

layout(location = 0) out vec3 frag_color;

layout(binding = 0) uniform UBO {
    mat4 model;
} ubo;

void main() {
    gl_Position = ubo.model * vec4(positions, 1.0);
    frag_color = colors;
}