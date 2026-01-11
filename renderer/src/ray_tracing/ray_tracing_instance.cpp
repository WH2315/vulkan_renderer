#include "ray_tracing/ray_tracing_instance.hpp"
#include "resources/normal_model.hpp"
#include "base/utils.hpp"
#include "manager.hpp"

namespace wen {

RayTracingInstance::RayTracingInstance() : instance_count_(0) {
    register_ = std::make_unique<CustomDataRegister<InstanceCreateInfo>>();
    register_->registerCustomData<InstanceAddress>();
    register_->registerCustomData<GLTFPrimitive::GLTFPrimitiveData>();
}

RayTracingInstance::~RayTracingInstance() {
    instance_buffer_.reset();
    manager->device->device.destroyAccelerationStructureKHR(tlas_, nullptr,
                                                            manager->dispatcher);
    buffer_.reset();
    scratch_.reset();
}

void RayTracingInstance::build(bool allow_update) {
    allow_update_ = allow_update;
    instance_count_ = register_->buildGroup();
    instance_buffer_ = std::make_unique<Buffer>(
        instance_count_ * sizeof(vk::AccelerationStructureInstanceKHR),
        vk::BufferUsageFlagBits::eShaderDeviceAddress |
            vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
        VMA_MEMORY_USAGE_AUTO, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);

    auto* ptr =
        static_cast<vk::AccelerationStructureInstanceKHR*>(instance_buffer_->map());
    for (auto& [id, group] : register_->groups) {
        uint32_t index = group.offset;
        auto* instance_ptr = ptr + index;
        for (auto& [binding, model, transform] : group.custom_data_cis) {
            instance_ptr->setInstanceCustomIndex(index)
                .setAccelerationStructureReference(
                    getAccelerationStructureAddress(model->blas_info.value()->blas))
                .setInstanceShaderBindingTableRecordOffset(binding)
                .setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable)
                .setMask(0xff)
                .setTransform(
                    convert<vk::TransformMatrixKHR, const glm::mat4&>(transform));
            instance_ptr++;
            index++;
        }
        group.custom_data_cis.clear();
    }

    // 将之前拷贝上传的实体设备内存进行设置打包
    vk::AccelerationStructureGeometryInstancesDataKHR geometry_instances = {};
    geometry_instances.setData(getBufferAddress(instance_buffer_->buffer));
    // 我们需要将实体数据放入联合体中并指定该数据为实体数据
    vk::AccelerationStructureGeometryKHR geometry = {};
    geometry.setGeometry(geometry_instances)
        .setGeometryType(vk::GeometryTypeKHR::eInstances);

    vk::AccelerationStructureBuildGeometryInfoKHR build = {};
    // 在构建加速结构时，优先考虑光线追踪的速度
    vk::BuildAccelerationStructureFlagsKHR flags =
        vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
    if (allow_update) {
        flags |= vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate;
    }
    build.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
        .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
        .setFlags(flags)
        .setGeometries(geometry);

    // 获取加速结构大小
    auto size_info = manager->device->device.getAccelerationStructureBuildSizesKHR(
        vk::AccelerationStructureBuildTypeKHR::eDevice, build, instance_count_,
        manager->dispatcher);

    // 创建 TLAS
    vk::AccelerationStructureCreateInfoKHR tlas_ci = {};
    buffer_ = std::make_unique<StorageBuffer>(
        size_info.accelerationStructureSize,
        vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
            vk::BufferUsageFlagBits::eShaderDeviceAddress,
        VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
    tlas_ci.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
        .setSize(size_info.accelerationStructureSize)
        .setBuffer(buffer_->getBuffer());
    tlas_ = manager->device->device.createAccelerationStructureKHR(tlas_ci, nullptr,
                                                                   manager->dispatcher);

    // 构建 TLAS
    auto cmdbuf = manager->command_pool->allocateSingleUse();
    scratch_ = std::make_unique<StorageBuffer>(
        size_info.buildScratchSize,
        vk::BufferUsageFlagBits::eStorageBuffer |
            vk::BufferUsageFlagBits::eShaderDeviceAddress,
        VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
    build.setDstAccelerationStructure(tlas_).setScratchData(
        getBufferAddress(scratch_->getBuffer()));
    vk::AccelerationStructureBuildRangeInfoKHR range = {};
    range.setPrimitiveCount(instance_count_)
        .setPrimitiveOffset(0)
        .setFirstVertex(0)
        .setTransformOffset(0);
    cmdbuf.buildAccelerationStructuresKHR(build, &range, manager->dispatcher);
    manager->command_pool->freeSingleUse(cmdbuf);
}

void RayTracingInstance::update(uint32_t id, FunUpdateInstance callback) {
    if (!allow_update_) {
        WEN_WARN("you had set allow_update to false, so you can't update the instance!")
        return;
    }

    register_->multiThreadUpdate(id, [=, this](uint32_t index, uint32_t begin,
                                               uint32_t end) {
        auto* instance_ptr =
            static_cast<vk::AccelerationStructureInstanceKHR*>(instance_buffer_->data);
        instance_ptr += begin;
        FunUpdateTransform update_transform = [&](const glm::mat4& transform) {
            instance_ptr->setTransform(
                convert<vk::TransformMatrixKHR, const glm::mat4&>(transform));
        };
        FunUpdataAccelerationStructure update_as = [&](std::shared_ptr<Model> model) {
            instance_ptr->setAccelerationStructureReference(
                getAccelerationStructureAddress(model->blas_info.value()->blas));
        };
        while (begin < end) {
            callback(index, update_transform, update_as);
            instance_ptr++;
            index++;
            begin++;
        }
    });

    vk::AccelerationStructureGeometryInstancesDataKHR geometry_instances = {};
    geometry_instances.setData(getBufferAddress(instance_buffer_->buffer));
    vk::AccelerationStructureGeometryKHR geometry = {};
    geometry.setGeometry(geometry_instances)
        .setGeometryType(vk::GeometryTypeKHR::eInstances);

    vk::AccelerationStructureBuildGeometryInfoKHR build = {};
    build.setType(vk::AccelerationStructureTypeKHR::eTopLevel)
        .setMode(vk::BuildAccelerationStructureModeKHR::eUpdate)
        .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace |
                  vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate)
        .setGeometries(geometry);

    auto size_info = manager->device->device.getAccelerationStructureBuildSizesKHR(
        vk::AccelerationStructureBuildTypeKHR::eDevice, build, instance_count_,
        manager->dispatcher);

    auto cmdbuf = manager->command_pool->allocateSingleUse();
    if (scratch_->getSize() < size_info.updateScratchSize) {
        scratch_.reset();
        scratch_ = std::make_unique<StorageBuffer>(
            size_info.updateScratchSize,
            vk::BufferUsageFlagBits::eStorageBuffer |
                vk::BufferUsageFlagBits::eShaderDeviceAddress,
            VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
    }
    build.setSrcAccelerationStructure(tlas_)
        .setDstAccelerationStructure(tlas_)
        .setScratchData(getBufferAddress(scratch_->getBuffer()));
    vk::AccelerationStructureBuildRangeInfoKHR range = {};
    range.setPrimitiveCount(instance_count_)
        .setPrimitiveOffset(0)
        .setFirstVertex(0)
        .setTransformOffset(0);
    cmdbuf.buildAccelerationStructuresKHR(build, &range, manager->dispatcher);
    manager->command_pool->freeSingleUse(cmdbuf);
}

InstanceAddress RayTracingInstance::createInstanceAddress(Model& model) {
    switch (model.getType()) {
        case Model::ModelType::eNormalModel:
            return {
                getBufferAddress(dynamic_cast<NormalModel&>(model)
                                     .ray_tracing_vertex_buffer->buffer),
                getBufferAddress(
                    dynamic_cast<NormalModel&>(model).ray_tracing_index_buffer->buffer),
            };
        case Model::ModelType::eGLTFPrimitive:
            return {
                getBufferAddress(dynamic_cast<GLTFPrimitive&>(model)
                                     .scene_.ray_tracing_vertex_buffer->buffer),
                getBufferAddress(dynamic_cast<GLTFPrimitive&>(model)
                                     .scene_.ray_tracing_index_buffer->buffer),
            };
        case Model::ModelType::eSphereModel:
            return {
                0,
                0,
            };
    }
    return {};
}

} // namespace wen