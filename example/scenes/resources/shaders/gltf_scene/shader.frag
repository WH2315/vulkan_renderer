#version 450

layout(location = 0) in vec2 position;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D image;

void main () {
    vec2 uv = position * 0.5 + 0.5;
    uv.y = 1 - uv.y;
    out_color = texture(image, uv);
}