#version 450

layout(location = 0) in vec2 ndc;

layout(location = 0) out vec4 out_color;

layout(binding = 0) uniform sampler2D image;

void main () {
    // 将 NDC 坐标 [-1, 1] 映射到纹理坐标 [0, 1]
    vec2 uv = ndc * 0.5 + 0.5;
    // 翻转 y 轴
    uv.y = 1 - uv.y;
    out_color = texture(image, uv);
}