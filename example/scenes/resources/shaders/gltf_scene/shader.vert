#version 450

vec2 positions[3] = vec2[](
    vec2(1.0, 1.0),
    vec2(-3.0, 1.0),
    vec2(1.0, -3.0)
);

layout(location = 0) out vec2 position;

void main() {
    position = positions[gl_VertexIndex];
    gl_Position = vec4(position, 0, 1);
}