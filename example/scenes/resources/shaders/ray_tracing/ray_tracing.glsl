#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable

struct InstanceAddress {
    uint64_t vertex_buffer_address;
    uint64_t index_buffer_address;
};

struct Vertex {
    vec3 position;
    float _pad0; // keep 16-byte alignment for physical storage buffer
    vec3 normal;
    float _pad1;
    vec3 color;
    float _pad2;
};

struct Index {
    uint i0;
    uint i1;
    uint i2;
};