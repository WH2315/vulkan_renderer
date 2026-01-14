#pragma once

#include "resources/vertex_input/vertex_buffer.hpp"

namespace wen {

constexpr size_t VOLUME_RAW_DATA_RESOLUTION = 640;

class VolumeData {
public:
    VolumeData(const std::string& filename);
    VolumeData(const std::vector<float>& raw_data);

    float get(int x, int y, int z) const;
    void set(int x, int y, int z, float value);

    void uploadToBuffer(std::shared_ptr<VertexBuffer> buffer);

private:
    int index(int x, int y, int z) const {
        return x * VOLUME_RAW_DATA_RESOLUTION * VOLUME_RAW_DATA_RESOLUTION +
               y * VOLUME_RAW_DATA_RESOLUTION + z;
    }

    std::vector<float> raw_data_;
};

} // namespace wen