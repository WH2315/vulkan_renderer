#include "resources/volume_data.hpp"

namespace wen {

VolumeData::VolumeData(const std::string& filename) {
    raw_data_.resize(VOLUME_RAW_DATA_RESOLUTION * VOLUME_RAW_DATA_RESOLUTION *
                     VOLUME_RAW_DATA_RESOLUTION);
    auto f = fopen(filename.c_str(), "rb");
    if (f == NULL) {
        return;
    }
    fread(raw_data_.data(), sizeof(float), raw_data_.size(), f);
    fclose(f);
}

VolumeData::VolumeData(const std::vector<float>& raw_data) : raw_data_(raw_data) {
    while (raw_data_.size() < VOLUME_RAW_DATA_RESOLUTION * VOLUME_RAW_DATA_RESOLUTION *
                                  VOLUME_RAW_DATA_RESOLUTION) {
        raw_data_.push_back(0);
    }
}

float VolumeData::get(int x, int y, int z) const {
    return raw_data_[index(x, y, z)];
}

void VolumeData::set(int x, int y, int z, float value) {
    raw_data_[index(x, y, z)] = value;
}

void VolumeData::uploadToBuffer(std::shared_ptr<VertexBuffer> buffer) {
    buffer->setData(raw_data_);
    buffer->flush();
}

} // namespace wen