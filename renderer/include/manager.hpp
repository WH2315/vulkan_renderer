#pragma once

#include "core/log.hpp"
#include "core/window.hpp"

namespace wen {

struct Manager {
    void initializeEngine();
    void initializeRenderer();
    bool shouldClose() const;
    void pollEvents() const;
    void destroyRenderer();
    void destroyEngine();
};

} // namespace wen