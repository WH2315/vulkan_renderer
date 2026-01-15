#version 450

layout(location = 0) in vec3 frag_pos;
layout(location = 1) in vec3 frag_normal;

layout(location = 0) out vec4 out_pos;
layout(location = 1) out vec4 out_normal;

void main() {
    // 使用片元的深度值作为w分量
    out_pos = vec4(frag_pos, gl_FragCoord.z);
    // 若此处有Fragment，则w分量设为1
    out_normal = vec4(normalize(frag_normal), 1);
}