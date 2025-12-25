#include "scenes.hpp"

void SceneManager::update() {
    scene_->update(ImGui::GetIO().DeltaTime);
}

void SceneManager::render() {
    scene_->renderer->acquireNextImage();
    scene_->renderer->beginRenderPass();

    scene_->render();
    scene_->imGui->begin();
    scene_->imgui();
    scene_->imGui->end();

    scene_->renderer->endRenderPass();
    scene_->renderer->present();
}

SceneManager::~SceneManager() {
    scene_->renderer->waitIdle();
    scene_->destroy();
    scene_.reset();
    interface_.reset();
}