#include "ray_tracing/sphere_model.hpp"
#include "ray_tracing/acceleration_structure.hpp"
#include "core/log.hpp"

namespace wen {

SphereModel::SphereModel() {
    register_ = std::make_unique<CustomDataRegister<uint8_t>>();
    register_->registerCustomData<SphereData>();
    as_ = std::make_unique<AccelerationStructure>();
}

SphereModel::~SphereModel() {
    register_.reset();
}

void SphereModel::build() {
    auto count = register_->buildGroup();
    if (count == 0) {
        WEN_ERROR("Not add any sphere model");
    }
    aabbs_.reserve(count);
    auto* ptr = static_cast<SphereData*>(getSphereDataBuffer()->map());
    for (uint32_t i = 0; i < count; ++i) {
        aabbs_.push_back({
            .min = ptr->center - ptr->radius,
            .max = ptr->center + ptr->radius,
        });
        ++ptr;
    }
    getSphereDataBuffer()->unmap();
    aabbs_buffer_ = std::make_shared<StorageBuffer>(
        aabbs_.size() * sizeof(SphereAABBData),
        vk::BufferUsageFlagBits::eShaderDeviceAddress |
            vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    memcpy(aabbs_buffer_->map(), aabbs_.data(), aabbs_buffer_->getSize());
    aabbs_buffer_->unmap();
}

void SphereModel::update(uint32_t id, FunUpdateCustomData<SphereData> callback) {
    register_->multiThreadUpdate(
        id, [=, this](uint32_t index, uint32_t begin, uint32_t end) {
            auto* sphere_data_ptr = static_cast<SphereData*>(
                register_->getCustomDataBuffer<SphereData>()->map());
            auto* aabb_data_ptr = static_cast<SphereAABBData*>(aabbs_buffer_->map());
            aabb_data_ptr += begin;
            sphere_data_ptr += begin;
            while (begin < end) {
                callback(index, *sphere_data_ptr);
                aabb_data_ptr->min = sphere_data_ptr->center - sphere_data_ptr->radius;
                aabb_data_ptr->max = sphere_data_ptr->center + sphere_data_ptr->radius;
                sphere_data_ptr++;
                aabb_data_ptr++;
                index++;
                begin++;
            }
        });
    std::shared_ptr<SphereModel> self(this, [](auto) {});
    as_->addModel(self);
    as_->build(true, true);
}

} // namespace wen