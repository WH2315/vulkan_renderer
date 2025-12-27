#include "scenes.hpp"

void SceneManager::applyPendingSceneChange() {
    if (pending_scene_index_ < 0 ||
        pending_scene_index_ >= static_cast<int>(scenes_.size())) {
        return;
    }
    if (pending_scene_index_ == active_scene_index_) {
        pending_scene_index_ = -1;
        return;
    }
    switchToScene(pending_scene_index_);
    pending_scene_index_ = -1;
}

void SceneManager::switchToScene(int index) {
    if (index < 0 || index >= static_cast<int>(scenes_.size())) {
        return;
    }

    if (scene_) {
        scene_->renderer->waitIdle();
        scene_->destroy();
        scene_.reset();
    }

    active_scene_index_ = index;
    scene_ = scenes_[index].factory();
    scene_->initialize();
}

void SceneManager::setActiveScene(const std::string& name) {
    for (int i = 0; i < static_cast<int>(scenes_.size()); ++i) {
        if (scenes_[i].name == name) {
            pending_scene_index_ = i;
            applyPendingSceneChange();
            return;
        }
    }
}

void SceneManager::renderSceneSelector() {
    if (scenes_.empty()) {
        return;
    }

    const char* current =
        active_scene_index_ >= 0 ? scenes_[active_scene_index_].name.c_str() : "(none)";
    if (ImGui::Begin("Scene Manager")) {
        if (ImGui::BeginCombo("Active Scene", current)) {
            for (int i = 0; i < static_cast<int>(scenes_.size()); ++i) {
                bool selected = (i == active_scene_index_);
                if (ImGui::Selectable(scenes_[i].name.c_str(), selected)) {
                    pending_scene_index_ = i;
                }
                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }
            ImGui::EndCombo();
        }
        ImGui::End();
    }
}

void SceneManager::update() {
    if (!scene_ && !scenes_.empty()) {
        switchToScene(0);
    }

    applyPendingSceneChange();

    if (scene_) {
        scene_->update(ImGui::GetIO().DeltaTime);
    }
}

void SceneManager::render() {
    if (!scene_) {
        return;
    }

    VkImageView view =
        scene_->renderer->framebuffer_set->attachments
            .at(scene_->renderer->render_pass->getAttachmentIndex(
                wen::IMGUI_DOCKING_ATTACHMENT, wen::renderer_config->msaa()))
            ->image_view;

    scene_->renderer->acquireNextImage();
    scene_->renderer->beginRenderPass();

    scene_->render();

    scene_->imGui->begin();

    auto* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGuiWindowFlags window_flags =
        ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoNavFocus;
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(3);
    ImGui::DockSpace(ImGui::GetID("DockSpace"), {0.0f, 0.0f}, 0, nullptr);

    // Scene Manager UI
    renderSceneSelector();

    // scene_->imgui();

    scene_->imGui->end();

    scene_->renderer->endRenderPass();
    scene_->renderer->present();
}

SceneManager::~SceneManager() {
    if (scene_) {
        scene_->renderer->waitIdle();
        scene_->destroy();
        scene_.reset();
    }
    interface_.reset();
}