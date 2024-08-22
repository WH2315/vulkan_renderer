#pragma once

#include "base/enums.hpp"

namespace wen {

struct VertexInputInfo {
    uint32_t binding;
    InputRate input_rate;
    std::vector<VertexType> formats;
};

class VertexInput {
    friend class RenderPipeline;

public:
    VertexInput(const std::vector<VertexInputInfo>& infos);
    ~VertexInput();

private:
    uint32_t location_ = 0;
    std::vector<vk::VertexInputAttributeDescription> attribute_descriptions_;
    std::vector<vk::VertexInputBindingDescription> binding_descriptions_;
};

} // namespace wen