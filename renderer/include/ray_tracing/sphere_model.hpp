#pragma once

#include "resources/model.hpp"
#include "ray_tracing/custom_data.hpp"
#include <glm/glm.hpp>

namespace wen {

class AccelerationStructure;
class SphereModel final : public Model {
    friend class AccelerationStructure;

public:
    struct SphereData {
        glm::vec3 center;
        float radius;
    };

    struct SphereAABBData {
        glm::vec3 min;
        glm::vec3 max;
    };

    SphereModel();
    ~SphereModel() override;

    template <class CustomSphereData>
    void registerCustomSphereData() {
        register_->registerCustomData<CustomSphereData>();
    }

    template <class ...CustomSphereDatas>
    void addSphereModel(uint32_t id, glm::vec3 center, float radius, CustomSphereDatas&&... datas) {
        register_->addInstance(
            id,
            0,
            SphereData{
                .center = center,
                .radius = radius
            },
            std::forward<CustomSphereDatas>(datas)...
        );
    }

    template <class CustomSphereData>
    std::shared_ptr<StorageBuffer> getCustomSphereDataBuffer() {
        return register_->getCustomDataBuffer<CustomSphereData>();
    }
    auto getSphereDataBuffer() {
        return getCustomSphereDataBuffer<SphereData>();
    }

    void build();

    template <class CustomShpereDatas>
    using FunUpdateCustomData = std::function<void(uint32_t, CustomShpereDatas&)>;
    void update(uint32_t id, FunUpdateCustomData<SphereData> callback);

    template <class CustomSphereData>
    void update(uint32_t id, FunUpdateCustomData<CustomSphereData> callback) {
        register_->multiThreadUpdate(id, [=, this](uint32_t index, uint32_t begin, uint32_t end) {
            auto* ptr = static_cast<CustomSphereData*>(getCustomSphereDataBuffer<CustomSphereData>()->map());
            ptr += begin;
            while (begin < end) {
                callback(index, *ptr);
                ptr++;
                index++;
                begin++;
            }
        });
    }

    ModelType getType() const override { return ModelType::eSphereModel; }

private:
    std::unique_ptr<CustomDataRegister<uint8_t>> register_;
    std::unique_ptr<AccelerationStructure> as_;
    std::vector<SphereAABBData> aabbs_;
    std::shared_ptr<StorageBuffer> aabbs_buffer_;
};

} // namespace wen