#version 450

layout(binding = 0) uniform sampler2D position_buffer;
layout(binding = 1, input_attachment_index = 1) uniform subpassInput normal_buffer;

layout(location = 0) out vec4 out_ssao_value;

layout(binding = 2) uniform SSAOSamples {
    vec4 samples[64];
} ssao;

layout(binding = 3) uniform sampler2D random_vectors_texture;

layout(binding = 4) uniform Camera {
    vec3 position;
    mat4 view;
    mat4 project;
} camera;

layout(push_constant) uniform PC {
    vec2 window_size;
    int a;
    int sample_count;
    float r;
} pc;

void main() {
    vec4 normal_s = subpassLoad(normal_buffer);
    // 此处没有片段，ssao值设为-1
    if (normal_s.w == 0) {
        out_ssao_value = vec4(-1.0);
        return;
    }
    // 这里的normal是世界空间下的法线
    vec3 normal = normal_s.xyz;
    // 这里的pos是世界空间下的位置
    vec3 pos = texture(position_buffer, gl_FragCoord.xy / pc.window_size).xyz;
    // random_vectors_texture 是噪声纹理，用于打破采样模式
    vec3 random_vector = texture(random_vectors_texture, gl_FragCoord.xy / float(pc.a)).xyz;
    // 把随机向量投影到法线切平面上并归一化，得到切线方向
    vec3 T = normalize(random_vector - dot(random_vector, normal) * normal);
    // 用法线和切线叉积得到副切线，确保三个轴正交
    vec3 B = cross(normal, T);
    // 组合成局部坐标系，把后续 SSAO 半球采样(TBN * sample)从切线空间旋转到世界空间，从而围绕当前表面法线分布样本
    mat3 TBN = mat3(T, B, normal);

    float ssao_value = 0;

    // 对每个样本做半球采样遮挡检测
    for (uint i = 0; i < pc.sample_count; ++i) {
        // 把单位半球样本从切线空间变换到世界空间，再乘半径并平移到当前像素位置
        vec3 sample_point = pos + pc.r * (TBN * ssao.samples[i].xyz);
        // 将样本点乘以 view 和 project 得到裁剪坐标
        vec4 cliped_point = camera.project * camera.view * vec4(sample_point, 1.0);
        // 透视除法再映射到 NDC [0,1]
        cliped_point.xyz /= cliped_point.w;
        cliped_point.xyz = cliped_point.xyz * 0.5 + 0.5;
        // 并翻转 y 以匹配纹理坐标系
        cliped_point.y = 1.0 - cliped_point.y;
        // 从位置缓冲读取对应屏幕坐标的深度值
        float depth = texture(position_buffer, cliped_point.xy).w;
        // 深度缓冲区深度小于样本点，说明样本点被几何遮挡
        if (depth < cliped_point.z) {
            ssao_value += 1.0;
        }
    }
    // 计算遮挡比例
    ssao_value = 1.0 - (ssao_value / float(pc.sample_count));
    out_ssao_value = vec4(ssao_value);
}