#version 450

layout(location = 0) in vec3 frag_dir;

layout(location = 0) out vec4 FragColor;

layout(binding = 1) uniform samplerCube skybox_tex;

void main() {
    FragColor = texture(skybox_tex, normalize(frag_dir));
}
