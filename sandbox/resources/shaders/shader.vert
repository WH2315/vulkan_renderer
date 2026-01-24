#version 450

layout(location = 0) in vec3 positions;
layout(location = 1) in vec3 normals;

layout(location = 0) out vec3 frag_position;
layout(location = 1) out vec3 frag_normal;
layout(location = 2) out vec3 frag_view_dir;
layout(location = 3) out vec3 frag_light_dir;

layout(binding = 0) uniform CameraData {
    vec3 position;
    mat4 view;
    mat4 projection;
} camera;

void main() {
    gl_Position = camera.projection * camera.view * vec4(positions, 1.0);
    frag_position = vec3(camera.view * vec4(positions, 1.0));
    frag_normal = mat3(camera.view) * normals;

	vec3 light_pos = vec3(0, 5, 5);
    frag_light_dir = light_pos - frag_position;
    frag_view_dir = -frag_position;
}