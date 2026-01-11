#include "ray_tracing/gltf/gltf_scene.hpp"
#include "resources/descriptor/descriptor_set.hpp"
#include "resources/descriptor/data_texture.hpp"
#include <glm/gtc/type_ptr.hpp>

namespace wen {

GLTFScene::GLTFScene(const std::string& filename,
                     const std::vector<std::string>& attrs) {
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err, warn;

    size_t pos = filename.find_last_of('/');
    filepath_ = filename.substr(0, pos);

    auto filetype = filename.substr(filename.find_last_of('.') + 1);
    bool ret = false;
    if (filetype == "gltf") {
        ret = loader.LoadASCIIFromFile(&model, &err, &warn, filename);
    } else if (filetype == "glb") {
        ret = loader.LoadBinaryFromFile(&model, &err, &warn, filename);
    } else {
        WEN_ERROR("unknown GLTF filetype {}", filename)
    }

    if (!warn.empty()) {
        WEN_WARN("warn: {} {}", warn, filename)
    }
    if (!err.empty()) {
        WEN_ERROR("err: {} {}", err, filename)
    }
    if (!ret) {
        WEN_CRITICAL("failed to load GLTF {}", filename)
        return;
    }

    for (auto& extension : model.extensionsRequired) {
        WEN_DEBUG("GLTF model required \"{}\" extension", extension)
    }

    loadImages(model);
    loadMaterials(model);
    loadMeshesAndPrimitives(model, attrs);
    loadAttributes();
    loadNodes(model);
}

void GLTFScene::build(
    std::function<void(GLTFNode*, std::shared_ptr<GLTFPrimitive>)> fun) {
    for (auto* node : nodes_ptr_) {
        if (node->getMesh() == nullptr) {
            continue;
        }
        for (auto& primitive : node->getMesh()->primitives) {
            fun(node, primitive);
        }
    }
}

void GLTFScene::bindTexturesSamplers(
    const std::shared_ptr<DescriptorSet>& descriptor_set, uint32_t binding) {
    std::vector<std::pair<std::shared_ptr<SpecificTexture>, std::shared_ptr<Sampler>>>
        textures_samplers;
    textures_samplers.reserve(textures_.size());
    for (auto& texture : textures_) {
        textures_samplers.emplace_back(texture, sampler_);
    }
    descriptor_set->bindTextures(binding, textures_samplers);
}

void GLTFScene::loadImages(const tinygltf::Model& model) {
    for (auto& image : model.images) {
        if (image.uri.empty()) {
            WEN_WARN("unsupported image format {}", image.name)
            continue;
        }

        std::string::size_type pos;
        if ((pos = image.uri.find_last_of('.')) != std::string::npos) {
            if (image.uri.substr(pos + 1) == "ktx") {
                // TODO:
                WEN_ERROR("")
                continue;
            }
        }

        std::vector<uint8_t> rgba;
        const uint8_t* data = nullptr;
        if (image.component == 3) {
            rgba.resize(image.width * image.height * 4);
            auto* rgb = image.image.data();
            auto* ptr = rgba.data();
            for (uint32_t i = 0; i < image.width * image.height; i++) {
                ptr[0] = rgb[0];
                ptr[1] = rgb[1];
                ptr[2] = rgb[2];
                ptr[3] = 0;
                ptr += 4;
                rgb += 3;
            }
            data = rgba.data();
            WEN_DEBUG("convert 3 channel(RGB) image to 4(RGBA) channel image {} X {}",
                      image.width, image.height)
        } else {
            assert(image.component == 4);
            data = image.image.data();
            WEN_DEBUG("use 4 channel(RGBA) image {} X {}", image.width, image.height)
        }
        textures_.push_back(
            std::make_shared<DataTexture>(data, image.width, image.height, 0));
    }
    sampler_ = std::make_shared<Sampler>(SamplerOptions{});
}

void GLTFScene::loadMaterials(const tinygltf::Model& model) {
    for (auto& material : model.materials) {
        auto& mat = materials_.emplace_back();
        mat.base_color_factor =
            glm::make_vec4(material.pbrMetallicRoughness.baseColorFactor.data());
        mat.base_color_texture = material.pbrMetallicRoughness.baseColorTexture.index;
        mat.emissive_factor = glm::make_vec3(material.emissiveFactor.data());
        mat.emissive_texture = material.emissiveTexture.index;
        mat.normal_texture = material.normalTexture.index;
        mat.metallic_factor = material.pbrMetallicRoughness.metallicFactor;
        mat.roughness_factor = material.pbrMetallicRoughness.roughnessFactor;
        mat.metallic_roughness_texture =
            material.pbrMetallicRoughness.metallicRoughnessTexture.index;
    }
    material_buffer_ = std::make_shared<StorageBuffer>(
        materials_.size() * sizeof(GLTFMaterial),
        vk::BufferUsageFlagBits::eStorageBuffer, VMA_MEMORY_USAGE_CPU_TO_GPU,
        VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
    auto* ptr = static_cast<uint8_t*>(material_buffer_->map());
    memcpy(ptr, materials_.data(), material_buffer_->getSize());
    material_buffer_->unmap();
}

void GLTFScene::loadMeshesAndPrimitives(const tinygltf::Model& model,
                                        const std::vector<std::string>& attrs) {
    for (const auto& mesh : model.meshes) {
        meshes_.push_back(std::make_shared<GLTFMesh>(*this, model, mesh, attrs));
    }
}

void GLTFScene::loadAttributes() {
    for (auto& [name, data] : attr_datas_) {
        attr_buffers_[name] = std::make_shared<StorageBuffer>(
            data.size(), vk::BufferUsageFlagBits::eStorageBuffer,
            VMA_MEMORY_USAGE_CPU_TO_GPU,
            VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT);
        auto* ptr = static_cast<uint8_t*>(attr_buffers_[name]->map());
        memcpy(ptr, data.data(), attr_buffers_[name]->getSize());
        attr_buffers_[name]->unmap();
    }
}

void GLTFScene::loadNodes(const tinygltf::Model& model) {
    auto default_scene = model.scenes[std::max(0, model.defaultScene)];
    for (auto index : default_scene.nodes) {
        auto node = std::make_shared<GLTFNode>(*this, model, index, nullptr);
        nodes_ptr_.push_back(node.get());
        nodes_.push_back(std::move(node));
    }
}

GLTFScene::~GLTFScene() {
    nodes_.clear();
    nodes_ptr_.clear();
    attr_datas_.clear();
    attr_buffers_.clear();
    meshes_.clear();
    materials_.clear();
    material_buffer_.reset();
    textures_.clear();
    sampler_.reset();
    vertices.clear();
    indices.clear();
    ray_tracing_vertex_buffer.reset();
    ray_tracing_index_buffer.reset();
}

} // namespace wen