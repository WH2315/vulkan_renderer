#pragma once

#include "base/utils.hpp"
#include <glslang/Public/ShaderLang.h>
#include <filesystem>

namespace wen {

class ShaderIncluder final : public glslang::TShader::Includer {
public:
    ShaderIncluder(const std::filesystem::path& filename) {
        filepath_ = filename.parent_path();
    }

    IncludeResult* includeLocal(const char* header_name, const char* includer_name,
                                size_t inclusion_depth) {
        auto data = readFile((filepath_ / std::string(header_name)).string());
        auto* contents = new std::string(data.data(), data.size());
        return new IncludeResult(header_name, contents->c_str(), contents->length(),
                                 static_cast<void*>(contents));
    }

    void releaseInclude(IncludeResult* result) {
        if (result) {
            delete static_cast<std::string*>(result->userData);
            delete result;
        }
    }

    ~ShaderIncluder() {}

private:
    std::filesystem::path filepath_;
};

} // namespace wen