#pragma once

#include "renderer.hpp"
#include <imgui.h>

namespace wen {

class Imgui {
public:
    Imgui(Renderer& renderer);
    ~Imgui();

    void begin();
    void end();

private:
    Renderer& renderer_;
    vk::DescriptorPool descriptor_pool_;
};

} // namespace wen