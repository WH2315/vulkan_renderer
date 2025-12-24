#pragma once

#include "core/log.hpp"
#include "core/window.hpp"
#include "base/configuration.hpp"
#include "context.hpp"

namespace wen {

struct Manager {
    void initializeEngine();
    void initializeRenderer();
    bool shouldClose() const;
    void pollEvents() const;
    void destroyRenderer();
    void destroyEngine();
};

// For Renderer (inner)
extern Context* manager;

} // namespace wen