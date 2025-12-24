#include "interface.hpp"

namespace wen {

Interface::Interface(const std::string& path) : path_(path) {
    shader_dir_ = path_ + "/shaders";
    texture_dir_ = path_ + "/textures";
}

std::shared_ptr<RenderPass> Interface::createRenderPass(bool auto_load) {
    return std::make_shared<RenderPass>(auto_load);
}

std::shared_ptr<Renderer> Interface::createRenderer(std::shared_ptr<RenderPass> render_pass) {
    return std::make_shared<Renderer>(render_pass);
}

std::shared_ptr<Shader> Interface::loadShader(const std::string& filename, ShaderStage stage) {
    return std::make_shared<Shader>(shader_dir_ + "/" + filename, stage);
}

std::shared_ptr<ShaderProgram> Interface::createShaderProgram() {
    return std::make_shared<ShaderProgram>();
}

std::shared_ptr<RenderPipeline> Interface::createRenderPipeline(std::weak_ptr<Renderer> renderer, std::shared_ptr<ShaderProgram> shader_program, const std::string& subpass_name) {
    return std::make_shared<RenderPipeline>(renderer, shader_program, subpass_name);
}

std::shared_ptr<VertexInput> Interface::createVertexInput(const std::vector<VertexInputInfo>& infos) {
    return std::make_shared<VertexInput>(infos);
}

std::shared_ptr<VertexBuffer> Interface::createVertexBuffer(uint32_t size, uint32_t count) {
    return std::make_shared<VertexBuffer>(size, count);
}

std::shared_ptr<IndexBuffer> Interface::createIndexBuffer(IndexType type, uint32_t count) {
    return std::make_shared<IndexBuffer>(type, count);
}

std::shared_ptr<DescriptorSet> Interface::createDescriptorSet() {
    return std::make_shared<DescriptorSet>();
}

std::shared_ptr<UniformBuffer> Interface::createUniformBuffer(uint64_t size) {
    return std::make_shared<UniformBuffer>(size);
}

std::shared_ptr<DataTexture> Interface::createTexture(const uint8_t* data, uint32_t width, uint32_t height, uint32_t mip_levels) {
    return std::make_shared<DataTexture>(data, width, height, mip_levels);
}

std::shared_ptr<ImageTexture> Interface::createTexture(const std::string& filename, uint32_t mip_levels) {
    auto pos = filename.find_last_of('.') + 1;
    auto filetype = filename.substr(pos, filename.size() - pos);
    std::string filepath = texture_dir_ + "/" + filename;
    if (filetype == "png" || filetype == "jpg") {
        return std::make_shared<ImageTexture>(filepath, mip_levels);
    }
    return nullptr;
}

std::shared_ptr<Sampler> Interface::createSampler(const SamplerOptions& options) {
    return std::make_shared<Sampler>(options);
}

std::shared_ptr<PushConstants> Interface::createPushConstants(ShaderStage stage, const std::list<std::pair<std::string, ConstantType>>& infos) {
    return std::make_shared<PushConstants>(stage, infos);
}

} // namespace wen