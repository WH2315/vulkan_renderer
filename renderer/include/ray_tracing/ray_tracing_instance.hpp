#pragma once

#include "ray_tracing/custom_data.hpp"
#include "ray_tracing/gltf/gltf_scene.hpp"

namespace wen {

struct InstanceAddress {
    vk::DeviceAddress vertex_buffer_address;
    vk::DeviceAddress index_buffer_address;
};

struct InstanceCreateInfo {
    uint32_t binding;
    std::shared_ptr<Model> model;
    glm::mat4 transform;
};

class RayTracingInstance {
    friend class DescriptorSet;

public:
    RayTracingInstance();
    ~RayTracingInstance();

    template <class CustomInstanceData>
    void registerCustomInstanceData() {
        register_->registerCustomData<CustomInstanceData>();
    }

    template <class ...Args>
    void addNormalModel(uint32_t id, uint32_t binding, std::shared_ptr<Model> model, const glm::mat4& matrix, Args&&... args) {
        register_->addInstance(
            id,
            {
                .binding = binding,
                .model = model,
                .transform = matrix
            },
            createInstanceAddress(*model),
            std::forward<Args>(args)...
        );
    }
    template <class ...Args>
    void addGLTFScene(uint32_t id, uint32_t binding, std::shared_ptr<GLTFScene> scene, Args&&... args) {
        scene->build([&](auto* node, auto primitive) {
            register_->addInstance(
                id,
                {
                    .binding = binding,
                    .model = primitive,
                    .transform = node->getWorldMatrix(),
                },
                createInstanceAddress(*primitive),
                primitive->getData(),
                std::forward<Args>(args)...
            );
        });
    }

    template <class CustomInstanceData>
    std::shared_ptr<StorageBuffer> getCustomInstanceDataBuffer() {
        return register_->getCustomDataBuffer<CustomInstanceData>();
    }
    auto getInstanceAddressBuffer() {
        return getCustomInstanceDataBuffer<InstanceAddress>();
    }
    auto getPrimitiveDataBuffer() {
        return getCustomInstanceDataBuffer<GLTFPrimitive::GLTFPrimitiveData>();
    }

    void build(bool allow_update);

    using FunUpdateTransform = std::function<void(const glm::mat4&)>;
    using FunUpdataAccelerationStructure = std::function<void(std::shared_ptr<Model>)>;
    using FunUpdateInstance = std::function<void(uint32_t, FunUpdateTransform, FunUpdataAccelerationStructure)>;
    void update(uint32_t id, FunUpdateInstance callback);

    template <class CustomInstanceData>
    using FunUpdateCustomData = std::function<void(uint32_t, CustomInstanceData&)>;
    template <class CustomInstanceData>
    void update(uint32_t id, FunUpdateCustomData<CustomInstanceData> callback) {
        register_->multiThreadUpdate(id, [=, this](uint32_t index, uint32_t begin, uint32_t end) {
            auto* ptr = static_cast<std::remove_reference_t<CustomInstanceData*>>(getCustomInstanceDataBuffer<CustomInstanceData>()->map());
            ptr += begin;
            while (begin < end) {
                callback(index, *ptr);
                ptr++;
                index++;
                begin++;
            }
        });
    }

private:
    static InstanceAddress createInstanceAddress(Model& model);

private:
    bool allow_update_;

    std::unique_ptr<CustomDataRegister<InstanceCreateInfo>> register_;
    uint32_t instance_count_;

    std::unique_ptr<Buffer> instance_buffer_;
    std::unique_ptr<StorageBuffer> buffer_;
    std::unique_ptr<StorageBuffer> scratch_;

    vk::AccelerationStructureKHR tlas_;
};

} // namespace wen