#include "ray_tracing/acceleration_structure.hpp"
#include "base/utils.hpp"
#include "core/log.hpp"

namespace wen {

AccelerationStructure::~AccelerationStructure() {
    staging_.reset();
    scratch_.reset();
}

void AccelerationStructure::addModel(std::shared_ptr<Model> model) {
    auto type = model->getType();
    switch (type) {
        case Model::ModelType::eNormalModel:
            models_.emplace_back(std::dynamic_pointer_cast<NormalModel>(model));
            break;
        case Model::ModelType::eGLTFPrimitive:
            WEN_ERROR("GLTFPrimitive is used to addGLTFScene!")
            break;
        case Model::ModelType::eSphereModel:
            sphere_models_.emplace_back(std::dynamic_pointer_cast<SphereModel>(model));
            break;
    }
}

void AccelerationStructure::addGLTFScene(std::shared_ptr<GLTFScene> scene) {
    scenes_.emplace_back(scene);
}

void AccelerationStructure::build(bool is_update, bool allow_update) {
    std::vector<AccelerationStructureInfo> infos;
    infos.reserve(models_.size() + scenes_.size() + sphere_models_.size());

    uint64_t max_staging_size = current_staging_size_,
             max_scratch_size = current_scratch_size_;
    auto mode = is_update ? vk::BuildAccelerationStructureModeKHR::eUpdate
                          : vk::BuildAccelerationStructureModeKHR::eBuild;
    auto flags = vk::BuildAccelerationStructureFlagBitsKHR::eAllowCompaction |
                 vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
    if (allow_update) {
        flags |= vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate;
    }

    // 将 normal_model 转变成光追几何体用于构建 blas
    for (auto& model : models_) {
        if (!is_update) {
            uint64_t vertices_size = model->vertex_count * sizeof(Vertex);
            uint64_t indices_size = model->index_count * sizeof(uint32_t);
            model->ray_tracing_vertex_buffer = std::make_unique<Buffer>(
                vertices_size,
                // 将数据或显存从其他缓冲区复制到此缓冲区
                vk::BufferUsageFlagBits::eTransferDst |
                    // 用于加速结构的只读输入
                    vk::BufferUsageFlagBits::
                        eAccelerationStructureBuildInputReadOnlyKHR |
                    // 用于在着色器中获取这个缓冲的地址来访问数据
                    vk::BufferUsageFlagBits::eShaderDeviceAddress,
                VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
            model->ray_tracing_index_buffer = std::make_unique<Buffer>(
                indices_size,
                vk::BufferUsageFlagBits::eTransferDst |
                    vk::BufferUsageFlagBits::
                        eAccelerationStructureBuildInputReadOnlyKHR |
                    vk::BufferUsageFlagBits::eShaderDeviceAddress,
                VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
            max_staging_size = std::max(max_staging_size, vertices_size);
            max_staging_size = std::max(max_staging_size, indices_size);
            model->blas_info = std::make_unique<ModelBLASInfo>();
        }
        auto& as_info = infos.emplace_back(*(model->blas_info.value()));
        uint32_t max_primitive_count = model->index_count / 3;

        // 将缓冲区描述为 VertexObj 的数组
        as_info.geometries
            .emplace_back()
            // Defines WHERE to read vertex/index data (device addresses) and HOW to
            // interpret it (format, stride, etc.)
            .triangles
            // 顶点的位置数据 : glm::vec3
            .setVertexFormat(vk::Format::eR32G32B32Sfloat)
            // 顶点数据的原内存地址
            .setVertexData(getBufferAddress(model->ray_tracing_vertex_buffer->buffer))
            // 顶点数据的偏移
            .setVertexStride(sizeof(Vertex))
            // 索引数据 : uint32_t
            .setIndexType(vk::IndexType::eUint32)
            .setIndexData(getBufferAddress(model->ray_tracing_index_buffer->buffer))
            .setMaxVertex(model->vertex_count - 1)
            .sType = vk::StructureType::eAccelerationStructureGeometryTrianglesDataKHR;
        // 将上述数据识别为包含不透明三角形
        // Wrapper that specifies WHAT type of geometry (triangles, instances, AABBs)
        // and build flags
        as_info.as_geometries.emplace_back()
            .setGeometry(as_info.geometries.back())
            .setGeometryType(vk::GeometryTypeKHR::eTriangles)
            .setFlags(vk::GeometryFlagBitsKHR::eOpaque);
        // 设置偏移
        // Defines WHICH portion of the data to process (primitive count, offsets, etc.)
        as_info.build_range_infos.emplace_back()
            .setFirstVertex(0)
            .setPrimitiveCount(max_primitive_count)
            .setPrimitiveOffset(0)
            .setTransformOffset(0);
        // 设置构建信息
        as_info.build_info.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
            .setMode(mode)
            .setFlags(flags)
            .setGeometries(as_info.as_geometries);
        // 获取构建大小信息
        as_info.size_info =
            manager->device->device.getAccelerationStructureBuildSizesKHR(
                vk::AccelerationStructureBuildTypeKHR::eDevice, as_info.build_info,
                max_primitive_count, manager->dispatcher);
        max_scratch_size =
            std::max(max_scratch_size, is_update ? as_info.size_info.updateScratchSize
                                                 : as_info.size_info.buildScratchSize);
    }
    // 将 GLTF 场景转变成光追几何体用于构建 blas
    for (auto& scene : scenes_) {
        if (!is_update) {
            uint64_t vertices_size = scene->vertices.size() * sizeof(glm::vec3);
            uint64_t indices_size = scene->indices.size() * sizeof(uint32_t);
            scene->ray_tracing_vertex_buffer = std::make_unique<Buffer>(
                vertices_size,
                vk::BufferUsageFlagBits::eTransferDst |
                    vk::BufferUsageFlagBits::
                        eAccelerationStructureBuildInputReadOnlyKHR |
                    vk::BufferUsageFlagBits::eShaderDeviceAddress,
                VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
            scene->ray_tracing_index_buffer = std::make_unique<Buffer>(
                indices_size,
                vk::BufferUsageFlagBits::eTransferDst |
                    vk::BufferUsageFlagBits::
                        eAccelerationStructureBuildInputReadOnlyKHR |
                    vk::BufferUsageFlagBits::eShaderDeviceAddress,
                VMA_MEMORY_USAGE_GPU_ONLY, VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
            max_staging_size = std::max(max_staging_size, vertices_size);
            max_staging_size = std::max(max_staging_size, indices_size);
        }
        scene->build([&](auto* node, auto primitive) {
            if (!is_update) {
                primitive->blas_info = std::make_unique<ModelBLASInfo>();
            }
            auto& as_info = infos.emplace_back(*primitive->blas_info.value());
            uint32_t max_primitive_count = primitive->index_count / 3;

            as_info.geometries.emplace_back()
                .triangles.setVertexFormat(vk::Format::eR32G32B32Sfloat)
                .setVertexData(
                    getBufferAddress(scene->ray_tracing_vertex_buffer->buffer))
                .setVertexStride(sizeof(glm::vec3))
                .setIndexType(vk::IndexType::eUint32)
                .setIndexData(getBufferAddress(scene->ray_tracing_index_buffer->buffer))
                .setMaxVertex(primitive->vertex_count - 1)
                .sType =
                vk::StructureType::eAccelerationStructureGeometryTrianglesDataKHR;
            as_info.as_geometries.emplace_back()
                .setGeometry(as_info.geometries.back())
                .setGeometryType(vk::GeometryTypeKHR::eTriangles)
                .setFlags(vk::GeometryFlagBitsKHR::eOpaque);
            as_info.build_range_infos.emplace_back()
                .setFirstVertex(primitive->getData().first_vertex)
                .setPrimitiveCount(max_primitive_count)
                .setPrimitiveOffset(primitive->getData().first_index * sizeof(uint32_t))
                .setTransformOffset(0);
            as_info.build_info.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
                .setMode(mode)
                .setFlags(flags)
                .setGeometries(as_info.as_geometries);
            as_info.size_info =
                manager->device->device.getAccelerationStructureBuildSizesKHR(
                    vk::AccelerationStructureBuildTypeKHR::eDevice, as_info.build_info,
                    max_primitive_count, manager->dispatcher);
            max_scratch_size = std::max(max_scratch_size,
                                        is_update ? as_info.size_info.updateScratchSize
                                                  : as_info.size_info.buildScratchSize);
        });
    }
    for (auto& model : sphere_models_) {
        if (!is_update) {
            if (model->aabbs_.empty()) {
                model->build();
            }
            model->blas_info = std::make_unique<ModelBLASInfo>();
        }
        auto max_primitive_count = static_cast<uint32_t>(model->aabbs_.size());
        auto& as_info = infos.emplace_back(*(model->blas_info.value()));

        as_info.geometries.emplace_back()
            .aabbs.setData(getBufferAddress(model->aabbs_buffer_->getBuffer()))
            .setStride(sizeof(SphereModel::SphereAABBData))
            .sType = vk::StructureType::eAccelerationStructureGeometryAabbsDataKHR;
        as_info.as_geometries.emplace_back()
            .setGeometry(as_info.geometries.back())
            .setGeometryType(vk::GeometryTypeKHR::eAabbs)
            .setFlags(vk::GeometryFlagBitsKHR::eOpaque);
        as_info.build_range_infos.emplace_back()
            .setFirstVertex(0)
            .setPrimitiveOffset(0)
            .setPrimitiveCount(max_primitive_count)
            .setTransformOffset(0);
        as_info.build_info.setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
            .setMode(mode)
            .setFlags(flags)
            .setGeometries(as_info.as_geometries);
        as_info.size_info =
            manager->device->device.getAccelerationStructureBuildSizesKHR(
                vk::AccelerationStructureBuildTypeKHR::eDevice, as_info.build_info,

                max_primitive_count, manager->dispatcher);
        max_scratch_size =
            std::max(max_scratch_size, is_update ? as_info.size_info.updateScratchSize
                                                 : as_info.size_info.buildScratchSize);
    }

    // create staging and scratch buffers if needed.
    if (max_staging_size > current_staging_size_) {
        staging_.reset();
        staging_ = std::make_unique<StorageBuffer>(
            max_staging_size,
            // 可以将这个缓冲区的数据复制到其他缓冲区或显存
            vk::BufferUsageFlagBits::eTransferSrc,
            // 将数据从 CPU 端传输到 GPU 端
            VMA_MEMORY_USAGE_CPU_TO_GPU,
            // 在 CPU 端对分配的内存进行顺序写入
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                // 允许使用传输队列执行传输操作
                VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT);
        current_staging_size_ = max_staging_size;
    }
    if (max_scratch_size > current_scratch_size_) {
        scratch_.reset();
        scratch_ = std::make_unique<StorageBuffer>(
            max_scratch_size,
            // 存储缓冲区是一种特殊类型的缓冲区，它可以在着色器中被读写
            vk::BufferUsageFlagBits::eShaderDeviceAddress |
                vk::BufferUsageFlagBits::eStorageBuffer,
            VMA_MEMORY_USAGE_GPU_ONLY,
            // 专用内存分配，以获得更好的性能
            VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
        current_scratch_size_ = max_scratch_size;
    }

    // upload data to GPU
    if (!is_update) {
        for (auto& model : models_) {
            auto* ptr = static_cast<uint8_t*>(staging_->map());

            uint64_t vertices_size = model->vertex_count * sizeof(Vertex);
            std::vector<Vertex> rt_vertices;
            rt_vertices.reserve(model->vertex_count);
            for (const auto& v : model->vertices()) {
                rt_vertices.push_back({v.position, v.normal, v.color});
            }
            memcpy(ptr, rt_vertices.data(), vertices_size);
            staging_->flush(vertices_size, model->ray_tracing_vertex_buffer->buffer);

            uint64_t indices_size = 0;
            for (const auto& [name, mesh] : model->meshes()) {
                uint64_t size = mesh->indices.size() * sizeof(uint32_t);
                memcpy(ptr, mesh->indices.data(), size);
                ptr += size;
                indices_size += size;
            }
            staging_->flush(indices_size, model->ray_tracing_index_buffer->buffer);
        }
        for (auto& scene : scenes_) {
            auto* ptr = static_cast<uint8_t*>(staging_->map());

            uint64_t vertices_size = scene->vertices.size() * sizeof(glm::vec3);
            memcpy(ptr, scene->vertices.data(), vertices_size);
            staging_->flush(vertices_size, scene->ray_tracing_vertex_buffer->buffer);

            uint64_t indices_size = scene->indices.size() * sizeof(uint32_t);
            memcpy(ptr, scene->indices.data(), indices_size);
            staging_->flush(indices_size, scene->ray_tracing_index_buffer->buffer);
        }
        if (staging_ != nullptr) {
            staging_->unmap();
        }
    }

    // 创建一个用于获取每一个 BLAS 压缩的存储大小的查询队列
    vk::QueryPoolCreateInfo query_pool_info{};
    query_pool_info.setQueryCount(static_cast<uint32_t>(infos.size()))
        .setQueryType(vk::QueryType::eAccelerationStructureCompactedSizeKHR);
    auto query_pool = manager->device->device.createQueryPool(query_pool_info);

    // 批量创建/压缩 BLAS，这样可以存入有限的内存
    std::vector<uint32_t> indices;
    vk::DeviceSize batch_size = {0};
    vk::DeviceSize batch_limit = {256 * 1024 * 1024}; // 256MB
    for (int i = 0; i < infos.size(); ++i) {
        // indices数组用于限制一次性创建底层加速结构的数量
        indices.push_back(i);
        batch_size += infos[i].size_info.accelerationStructureSize;
        // 超过 256MB 或是最后一个 BLAS
        if (batch_size > batch_limit || i == infos.size() - 1) {
            auto cmdbuf = manager->command_pool->allocateSingleUse();
            cmdbuf.resetQueryPool(query_pool, 0, infos.size());
            if (!is_update) {
                uint32_t index = 0;
                for (auto j : indices) {
                    auto& as_info = infos[j];
                    // 真正的缓存分配和加速结构创建
                    as_info.buffer = std::make_unique<StorageBuffer>(
                        as_info.size_info.accelerationStructureSize,
                        // 可以将这个缓冲区的数据用于存储加速结构的数据
                        vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
                            vk::BufferUsageFlagBits::eShaderDeviceAddress,
                        VMA_MEMORY_USAGE_GPU_ONLY,
                        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);
                    vk::AccelerationStructureCreateInfoKHR as_create_info{};
                    as_create_info
                        .setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
                        .setBuffer(as_info.buffer->getBuffer())
                        .setSize(as_info.size_info.accelerationStructureSize);
                    as_info.blas =
                        manager->device->device.createAccelerationStructureKHR(
                            as_create_info, nullptr, manager->dispatcher);
                    // 构建 BLAS
                    as_info.build_info.setDstAccelerationStructure(as_info.blas)
                        .setScratchData(getBufferAddress(scratch_->getBuffer()));
                    cmdbuf.buildAccelerationStructuresKHR(
                        as_info.build_info, as_info.build_range_infos.data(),
                        manager->dispatcher);

                    // 一旦暂付缓存被重复使用,
                    // 我们需要一个栅栏用于确保之前的构建已经结束才开始构建下一个
                    vk::MemoryBarrier barrier = {};
                    barrier
                        .setSrcAccessMask(
                            vk::AccessFlagBits::eAccelerationStructureWriteKHR)
                        .setDstAccessMask(
                            vk::AccessFlagBits::eAccelerationStructureReadKHR);
                    cmdbuf.pipelineBarrier(
                        vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                        vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                        vk::DependencyFlags{}, barrier, {}, {});
                    // 查询真正需要的内存数量，用于压缩
                    cmdbuf.writeAccelerationStructuresPropertiesKHR(
                        as_info.blas,
                        vk::QueryType::eAccelerationStructureCompactedSizeKHR,
                        query_pool, index, manager->dispatcher);
                    index++;
                }
                manager->command_pool->freeSingleUse(cmdbuf);

                /*
                    大体上来说，压缩流程如下：
                    1. 获取查询到的数据（压缩大小）
                    2. 使用较小的大小创建一个新的加速结构
                    3. 将之前的加速结构拷贝到新创建的加速结构中
                    4. 将之前的加速结构销毁
                */
                // 获取查询到的数据（压缩大小）
                std::vector<vk::DeviceSize> compacted_sizes(indices.size());
                auto result = manager->device->device.getQueryPoolResults(
                    query_pool, 0, static_cast<uint32_t>(indices.size()),
                    sizeof(vk::DeviceSize) * compacted_sizes.size(),
                    compacted_sizes.data(), sizeof(vk::DeviceSize),
                    vk::QueryResultFlagBits::eWait | vk::QueryResultFlagBits::e64);
                if (result != vk::Result::eSuccess) {
                    WEN_ERROR("Failed to get query pool results.")
                }

                cmdbuf = manager->command_pool->allocateSingleUse();
                index = 0;
                for (auto j : indices) {
                    auto& as_info = infos[j];
                    // 创建压缩版本的加速结构
                    auto& model_blas_info = as_info.model_blas_info;
                    model_blas_info.buffer = std::make_unique<StorageBuffer>(
                        compacted_sizes[index],
                        vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
                            vk::BufferUsageFlagBits::eShaderDeviceAddress,
                        VMA_MEMORY_USAGE_GPU_ONLY,
                        VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT);

                    vk::AccelerationStructureCreateInfoKHR as_create_info{};
                    as_create_info
                        .setType(vk::AccelerationStructureTypeKHR::eBottomLevel)
                        .setBuffer(model_blas_info.buffer->getBuffer())
                        .setSize(compacted_sizes[index]);
                    model_blas_info.blas =
                        manager->device->device.createAccelerationStructureKHR(
                            as_create_info, nullptr, manager->dispatcher);

                    // 将之前的 BLAS 拷贝至压缩版本中
                    vk::CopyAccelerationStructureInfoKHR compact{};
                    compact.setSrc(as_info.blas)
                        .setDst(model_blas_info.blas)
                        .setMode(vk::CopyAccelerationStructureModeKHR::eCompact);
                    cmdbuf.copyAccelerationStructureKHR(compact, manager->dispatcher);
                    index++;
                }
            } else {
                for (auto j : indices) {
                    auto& as_info = infos[j];
                    auto& model_blas_info = as_info.model_blas_info;
                    as_info.build_info.setSrcAccelerationStructure(model_blas_info.blas)
                        .setDstAccelerationStructure(model_blas_info.blas)
                        .setScratchData(getBufferAddress(scratch_->getBuffer()));
                    cmdbuf.buildAccelerationStructuresKHR(
                        as_info.build_info, as_info.build_range_infos.data(),
                        manager->dispatcher);

                    vk::MemoryBarrier barrier = {};
                    barrier
                        .setSrcAccessMask(
                            vk::AccessFlagBits::eAccelerationStructureWriteKHR)
                        .setDstAccessMask(
                            vk::AccessFlagBits::eAccelerationStructureReadKHR);
                    cmdbuf.pipelineBarrier(
                        vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                        vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                        vk::DependencyFlags{}, barrier, {}, {});
                }
            }
            manager->command_pool->freeSingleUse(cmdbuf);

            // 重置批量处理数据
            batch_size = 0;
            indices.clear();
        }
    }

    if (!is_update) {
        for (auto& as_info : infos) {
            manager->device->device.destroyAccelerationStructureKHR(
                as_info.blas, nullptr, manager->dispatcher);
            as_info.buffer.reset();
        }
    }
    infos.clear();
    manager->device->device.destroyQueryPool(query_pool);
    models_.clear();
    scenes_.clear();
    sphere_models_.clear();
}

} // namespace wen