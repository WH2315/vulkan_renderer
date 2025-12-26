#pragma once

#include <glm/glm.hpp>

namespace wen {

class Model {
public:
    enum class ModelType {
        eNormalModel,
        eGLTFPrimitive,
    };

public:
    virtual ModelType getType() const = 0;
    virtual ~Model() {}
};

} // namespace wen