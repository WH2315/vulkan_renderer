#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable

struct InstanceAddress {
    uint64_t vertex_buffer_address;
    uint64_t index_buffer_address;
};

struct Vertex {
    vec3 position;
    vec3 normal;
    vec3 color;
};

struct Index {
    uint i0;
    uint i1;
    uint i2;
};