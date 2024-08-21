#include "resources/shader.hpp"
#include "base/utils.hpp"
#include "manager.hpp"

#include <glslang/Public/ShaderLang.h>
#include <glslang/SPIRV/GlslangToSpv.h>
#include <glslang/Public/ResourceLimits.h>

namespace wen {

Shader::Shader(const std::string& filename, ShaderStage stage) {
    auto code = readFile(filename);
    this->stage = stage;

    EShLanguage shader_stage;
    switch (stage) {
        case ShaderStage::eVertex:
            shader_stage = EShLangVertex;
            break;
        case ShaderStage::eFragment:
            shader_stage = EShLangFragment;
            break;
    }
    glslang::TShader shader(shader_stage);
    auto data = code.data();
    int version = 450;
    shader.setStrings(&data, 1);
    shader.setEnvInput(glslang::EShSourceGlsl, shader_stage, glslang::EShClientVulkan, version);
    shader.setEnvClient(glslang::EShClientVulkan, glslang::EShTargetVulkan_1_3);
    shader.setEnvTarget(glslang::EShTargetSpv, glslang::EShTargetSpv_1_6);
    if (!shader.parse(GetDefaultResources(), version, ENoProfile, false, false, EShMessages::EShMsgDefault)) {
        WEN_ERROR("{}, {}", shader.getInfoLog(), shader.getInfoDebugLog())
        return;
    }
    glslang::TProgram program;
    program.addShader(&shader);
    if (!program.link(EShMessages::EShMsgDefault)) {
        WEN_ERROR("{}, {}", shader.getInfoLog(), shader.getInfoDebugLog())
        return;
    }
    const auto intermediate = program.getIntermediate(shader_stage);
    std::vector<uint32_t> spirv;
    glslang::GlslangToSpv(*intermediate, spirv);

    vk::ShaderModuleCreateInfo create_info;
    create_info.setCodeSize(spirv.size() * sizeof(uint32_t))
        .setPCode(spirv.data());
    module = manager->device->device.createShaderModule(create_info);
}

Shader::~Shader() {
    if (module.has_value()) {
        manager->device->device.destroyShaderModule(module.value());
        module.reset();
    }
}

} // namespace wen