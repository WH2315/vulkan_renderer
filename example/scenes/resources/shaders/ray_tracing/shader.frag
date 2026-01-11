#version 450

layout(binding = 0) uniform sampler2D image;

layout(binding = 0, set = 1) uniform Info {
    vec2 window_size;
    vec3 clear_color;
} info;

layout(location = 0) out vec4 out_color;

void main() {
    vec2 uv = gl_FragCoord.xy / info.window_size;
    uv.y = 1 - uv.y;
    out_color = texture(image, uv);
}