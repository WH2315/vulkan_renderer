#version 450

layout(location = 0) in vec2 frag_uv;

layout(location = 0) out vec4 FragColor;

layout(binding = 0) uniform sampler2D tex;

void main() {
    FragColor = vec4(texture(tex, frag_uv).rgb, 1.0);
}