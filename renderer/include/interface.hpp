#pragma once

#include "resources/render_pass.hpp"
#include "renderer.hpp"
#include "resources/shader.hpp"
#include "resources/shader_program.hpp"
#include "resources/render_pipeline.hpp"
#include "resources/vertex_input/vertex_input.hpp"
#include "resources/vertex_input/vertex_buffer.hpp"
#include "resources/vertex_input/index_buffer.hpp"
#include "resources/descriptor/descriptor_set.hpp"
#include "resources/descriptor/uniform_buffer.hpp"
#include "resources/descriptor/image_texture.hpp"
#include "resources/sampler.hpp"
#include "resources/push_constants/push_constants.hpp"
#include "resources/normal_model.hpp"
#include "resources/descriptor/storage_image.hpp"
#include "ray_tracing/shader_program.hpp"
#include "ray_tracing/render_pipeline.hpp"
#include "ray_tracing/acceleration_structure.hpp"
#include "ray_tracing/ray_tracing_instance.hpp"
#include "ray_tracing/gltf/gltf_scene.hpp"
#include "ray_tracing/sphere_model.hpp"

namespace wen {

class Interface {
public:
    Interface(const std::string& path);

    std::shared_ptr<RenderPass> createRenderPass(bool auto_load = true);
    std::shared_ptr<Renderer> createRenderer(std::shared_ptr<RenderPass> render_pass);
    std::shared_ptr<Shader> loadShader(const std::string& filename, ShaderStage stage);
    std::shared_ptr<GraphicsShaderProgram> createGraphicsShaderProgram();
    std::shared_ptr<GraphicsRenderPipeline> createGraphicsRenderPipeline(std::weak_ptr<Renderer> renderer, std::shared_ptr<GraphicsShaderProgram> shader_program, const std::string& subpass_name);
    std::shared_ptr<VertexInput> createVertexInput(const std::vector<VertexInputInfo>& infos);
    std::shared_ptr<VertexBuffer> createVertexBuffer(uint32_t size, uint32_t count);
    std::shared_ptr<IndexBuffer> createIndexBuffer(IndexType type, uint32_t count);
    std::shared_ptr<DescriptorSet> createDescriptorSet();
    std::shared_ptr<UniformBuffer> createUniformBuffer(uint64_t size);
    std::shared_ptr<DataTexture> createTexture(const uint8_t* data, uint32_t width, uint32_t height, uint32_t mip_levels = 0);
    std::shared_ptr<ImageTexture> createTexture(const std::string& filename, uint32_t mip_levels = 0);
    std::shared_ptr<Sampler> createSampler(const SamplerOptions& options = {});
    std::shared_ptr<PushConstants> createPushConstants(ShaderStages stages, const std::list<std::pair<std::string, ConstantType>>& infos);
    std::shared_ptr<NormalModel> loadNormalModel(const std::string& filename, const std::vector<std::string>& blacklist = {});
    std::shared_ptr<StorageImage> createStorageImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageUsageFlags usage = {});
    std::shared_ptr<RayTracingShaderProgram> createRayTracingShaderProgram();
    std::shared_ptr<RayTracingRenderPipeline> createRayTracingRenderPipeline(std::shared_ptr<RayTracingShaderProgram> shader_program);
    std::shared_ptr<AccelerationStructure> createAccelerationStructure();
    std::shared_ptr<RayTracingInstance> createRayTracingInstance();
    std::shared_ptr<GLTFScene> loadGLTFScene(const std::string& filename, const std::vector<std::string>& attrs = {});
    std::shared_ptr<SphereModel> createSphereModel();

private:
    std::string path_;
    std::string shader_dir_;
    std::string texture_dir_;
    std::string model_dir_;
    std::string gltf_dir_;
};

} // namespace wen