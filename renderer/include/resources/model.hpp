#pragma once

#include "resources/descriptor/storage_buffer.hpp"
#include "manager.hpp"
#include <optional>

namespace wen {

struct ModelBLASInfo {
    ModelBLASInfo() = default;

    ~ModelBLASInfo() {
        manager->device->device.destroyAccelerationStructureKHR(blas, nullptr,
                                                                manager->dispatcher);
        buffer.reset();
    }

    std::unique_ptr<StorageBuffer> buffer;
    vk::AccelerationStructureKHR blas = nullptr;
};

class Model {
public:
    enum class ModelType {
        eNormalModel,
        eGLTFPrimitive,
        eSphereModel
    };

public:
    virtual ModelType getType() const = 0;

    virtual ~Model() {
        if (blas_info.has_value()) {
            blas_info.reset();
        }
    }

    std::optional<std::unique_ptr<ModelBLASInfo>> blas_info{std::nullopt};
};

} // namespace wen