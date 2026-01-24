#version 450

layout(location = 0) in vec3 positions;

layout(location = 0) out vec3 frag_dir;

layout(binding = 0) uniform CameraData {
    vec3 position;
    mat4 view;
    mat4 projection;
} camera;

void main() {
    frag_dir = positions;
    frag_dir.x *= -1.0;
    gl_Position = camera.projection * mat4(mat3(camera.view)) * vec4(positions, 1.0);
}
