#pragma once

#include "resources/normal_model.hpp"
#include "ray_tracing/gltf/gltf_scene.hpp"

namespace wen {

struct AccelerationStructureInfo {
    AccelerationStructureInfo(ModelBLASInfo& model_blas_info)
        : model_blas_info(model_blas_info) {}

    ModelBLASInfo& model_blas_info;
    std::unique_ptr<StorageBuffer> buffer{};
    vk::AccelerationStructureKHR blas = nullptr;
    std::vector<vk::AccelerationStructureGeometryDataKHR> geometries{};
    std::vector<vk::AccelerationStructureGeometryKHR> as_geometries{};
    std::vector<vk::AccelerationStructureBuildRangeInfoKHR> build_range_infos{};
    vk::AccelerationStructureBuildGeometryInfoKHR build_info{};
    vk::AccelerationStructureBuildSizesInfoKHR size_info{};
};

class AccelerationStructure {
public:
    AccelerationStructure() = default;
    ~AccelerationStructure();
    void addModel(std::shared_ptr<Model> model);
    void addGLTFScene(std::shared_ptr<GLTFScene> scene);
    void build(bool is_update, bool allow_update);

private:
    std::unique_ptr<StorageBuffer> staging_ = {};
    uint64_t current_staging_size_ = 0;
    std::unique_ptr<StorageBuffer> scratch_ = {};
    uint64_t current_scratch_size_ = 0;
    std::vector<std::shared_ptr<NormalModel>> models_ = {};
    std::vector<std::shared_ptr<GLTFScene>> scenes_ = {};
};

} // namespace wen