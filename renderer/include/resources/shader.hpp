#pragma once

#include "base/enums.hpp"
#include <optional>

namespace wen {

class Shader final {
public:
    Shader(const std::string& filename, ShaderStage stage);
    ~Shader();

    ShaderStage stage;
    std::optional<vk::ShaderModule> module;
};

} // namespace wen