#version 450

layout(location = 0) in vec3 frag_color;
layout(location = 1) in vec2 frag_uv;

layout(location = 0) out vec4 FragColor;

layout(binding = 1) uniform sampler2D tex;

void main() {
    FragColor = vec4(texture(tex, frag_uv * 2.0).rgb, 1.0);
}