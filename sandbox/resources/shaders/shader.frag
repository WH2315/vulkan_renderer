#version 450

layout(location = 0) in vec3 frag_position;
layout(location = 1) in vec3 frag_normal;
layout(location = 2) in vec3 frag_view_dir;
layout(location = 3) in vec3 frag_light_dir;

layout(location = 0) out vec4 FragColor;

layout(binding = 0) uniform CameraData {
    vec3 position;
    mat4 view;
    mat4 projection;
} camera;

layout(binding = 1) uniform samplerCube sampler_color;

void main() {
    vec3 N = normalize(frag_normal);
    vec3 L = normalize(frag_light_dir);
    vec3 V = normalize(frag_view_dir);
    vec3 R = reflect(-L, N);

	vec3 cI = normalize(frag_position);
	vec3 cR = reflect(cI, N);
    cR = vec3(inverse(camera.view) * vec4(cR, 0.0));
	cR.x *= -1.0;

	vec4 color = texture(sampler_color, cR);
    vec3 ambient = vec3(0.5) * color.rgb;
	vec3 diffuse = max(dot(N, L), 0.0) * vec3(1.0);
	vec3 specular = pow(max(dot(R, V), 0.0), 16.0) * vec3(0.5);
	FragColor = vec4(ambient + diffuse * color.rgb + specular, 1.0);
}