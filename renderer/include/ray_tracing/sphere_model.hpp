#pragma once

#include "resources/model.hpp"
#include <glm/glm.hpp>

namespace wen {

class SphereModel final : public Model {
public:
    struct SphereData {
        glm::vec3 center;
        float radius;
    };

    SphereModel();
    ~SphereModel() override;

    ModelType getType() const override { return ModelType::eSphereModel; }
};

} // namespace wen