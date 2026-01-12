#version 450

/*
这里用的是全屏三角形技巧：
在 clip/NDC 空间里把三个顶点放到屏幕外，让一大块三角形覆盖整个视口。
gl_Position 是 clip 空间坐标，超出 ±1 的部分会被裁剪
*/
vec2 positions[3] = vec2[](
    vec2(1.0, 1.0),
    vec2(-3.0, 1.0),
    vec2(1.0, -3.0)
);

layout(location = 0) out vec2 ndc;

void main() {
    ndc = positions[gl_VertexIndex];
    gl_Position = vec4(ndc, 0, 1);
}