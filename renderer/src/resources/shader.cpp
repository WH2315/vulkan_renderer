#include "resources/shader.hpp"
#include "base/utils.hpp"
#include "manager.hpp"

namespace wen {

Shader::Shader(const std::string& filename, ShaderStage stage) {
    auto code = readFile(filename);
    this->stage = stage;
    vk::ShaderModuleCreateInfo create_info;
    create_info.setCodeSize(code.size())
        .setPCode(reinterpret_cast<const uint32_t*>(code.data()));
    module = manager->device->device.createShaderModule(create_info);
}

Shader::~Shader() {
    if (module.has_value()) {
        manager->device->device.destroyShaderModule(module.value());
        module.reset();
    }
}

} // namespace wen