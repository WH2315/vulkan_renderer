#pragma once

#include <wen.hpp>
#include <glm/glm.hpp>
#include <functional>
#include <string>
#include <vector>
#include "core/imgui.hpp"

class Scene {
    friend class SceneManager;

public:
    Scene(std::shared_ptr<wen::Interface> interface) : interface(interface) {}

    virtual ~Scene() = default;

    virtual void initialize() = 0;
    virtual void update(float ts, float w, float h) = 0;
    virtual void render(float w, float h) = 0;
    virtual void imgui() = 0;
    virtual void destroy() = 0;

protected:
    std::shared_ptr<wen::Interface> interface;
    std::shared_ptr<wen::Renderer> renderer;
    std::shared_ptr<wen::Imgui> imGui;
    glm::vec2 viewport_size{static_cast<float>(wen::renderer_config->getWidth()),
                            static_cast<float>(wen::renderer_config->getHeight())};
};

class SceneManager {
public:
    SceneManager(std::shared_ptr<wen::Interface> interface) : interface_(interface) {}

    ~SceneManager();

    template <class Scene>
    void addScene(const std::string& name) {
        scenes_.push_back(
            {name, [this]() { return std::make_unique<Scene>(interface_); }});
    }

    void setActiveScene(const std::string& name);

    void update();
    void render();

    VkDescriptorSet image = VK_NULL_HANDLE;
    VkImageView last_view = VK_NULL_HANDLE;
    VkSampler last_sampler = VK_NULL_HANDLE;

private:
    struct SceneEntry {
        std::string name;
        std::function<std::unique_ptr<Scene>()> factory;
    };

    void applyPendingSceneChange();
    void switchToScene(int index);

    std::shared_ptr<wen::Interface> interface_;
    std::unique_ptr<Scene> scene_;
    std::vector<SceneEntry> scenes_;
    int active_scene_index_ = -1;
    int pending_scene_index_ = -1;

    std::shared_ptr<wen::Sampler> docking_sampler_;
};