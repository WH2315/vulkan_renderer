#include "scenes.hpp"
#include <backends/imgui_impl_vulkan.h>
#include <algorithm>

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
        if (image != VK_NULL_HANDLE) {
            ImGui_ImplVulkan_RemoveTexture(image);
            image = VK_NULL_HANDLE;
            last_view = VK_NULL_HANDLE;
            last_sampler = VK_NULL_HANDLE;
        }
        scene_->destroy();
        scene_.reset();
    }

    active_scene_index_ = index;
    scene_ = scenes_[index].factory();
    scene_->initialize();
    scene_->viewport_size = {static_cast<float>(wen::renderer_config->getWidth()),
                             static_cast<float>(wen::renderer_config->getHeight())};
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

void SceneManager::update() {
    if (!scene_ && !scenes_.empty()) {
        switchToScene(0);
    }

    applyPendingSceneChange();
}

void SceneManager::render() {
    if (!scene_) {
        return;
    }

    if (!docking_sampler_) {
        docking_sampler_ = interface_->createSampler(
            {.mag_filter = vk::Filter::eLinear,
             .min_filter = vk::Filter::eLinear,
             .address_mode_u = vk::SamplerAddressMode::eClampToEdge,
             .address_mode_v = vk::SamplerAddressMode::eClampToEdge,
             .address_mode_w = vk::SamplerAddressMode::eClampToEdge,
             .border_color = vk::BorderColor::eFloatOpaqueBlack,
             .mipmap_mode = vk::SamplerMipmapMode::eLinear,
             .mip_levels = 1});
    }

    VkImageView view =
        scene_->renderer->framebuffer_set->attachments
            .at(scene_->renderer->render_pass->getAttachmentIndex(
                wen::IMGUI_DOCKING_ATTACHMENT, wen::renderer_config->msaa()))
            ->image_view;

    if (image == VK_NULL_HANDLE || view != last_view ||
        docking_sampler_->sampler != last_sampler) {
        if (image != VK_NULL_HANDLE) {
            ImGui_ImplVulkan_RemoveTexture(image);
        }
        image = ImGui_ImplVulkan_AddTexture(docking_sampler_->sampler, view,
                                            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        last_view = view;
        last_sampler = docking_sampler_->sampler;
    }

    scene_->renderer->acquireNextImage();

    float dt = ImGui::GetIO().DeltaTime;
    const auto framebuffer_w = static_cast<float>(wen::renderer_config->getWidth());
    const auto framebuffer_h = static_cast<float>(wen::renderer_config->getHeight());

    scene_->imGui->newFrame();

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

    // render Scene Manager UI
    const char* current =
        active_scene_index_ >= 0 ? scenes_[active_scene_index_].name.c_str() : "(none)";
    bool scene_manager_open = ImGui::Begin("Scene Manager");
    if (scene_manager_open) {
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
    }
    ImGui::End();
    // render Settings UI
    scene_->imgui();
    // render Viewport UI
    bool viewport_open = ImGui::Begin("Viewport");
    if (viewport_open) {
        auto viewport_available = ImGui::GetContentRegionAvail();
        scene_->viewport_size.x = std::max(1.0f, viewport_available.x);
        scene_->viewport_size.y = std::max(1.0f, viewport_available.y);
        auto viewport_w = std::clamp(scene_->viewport_size.x, 1.0f, framebuffer_w);
        auto viewport_h = std::clamp(scene_->viewport_size.y, 1.0f, framebuffer_h);
        scene_->update(dt, viewport_w, viewport_h);
        if (!scene_->is_enable_ray_tracing) {
            scene_->renderer->beginRenderPass();
        }
        scene_->render(viewport_w, viewport_h);
        ImVec2 uv1{std::min(viewport_w / framebuffer_w, 1.0f),
                   std::min(viewport_h / framebuffer_h, 1.0f)};
        ImGui::Image(image, ImVec2{scene_->viewport_size.x, scene_->viewport_size.y},
                     ImVec2{0.0f, 0.0f}, uv1);
    }
    ImGui::End();

    ImGui::End();

    scene_->imGui->renderFrame();

    scene_->renderer->endRenderPass();
    scene_->renderer->present();
}

SceneManager::~SceneManager() {
    if (scene_) {
        scene_->renderer->waitIdle();
        if (image != VK_NULL_HANDLE) {
            ImGui_ImplVulkan_RemoveTexture(image);
            image = VK_NULL_HANDLE;
            last_view = VK_NULL_HANDLE;
            last_sampler = VK_NULL_HANDLE;
        }
        scene_->destroy();
        scene_.reset();
    }
    if (image != VK_NULL_HANDLE) {
        ImGui_ImplVulkan_RemoveTexture(image);
        image = VK_NULL_HANDLE;
        last_view = VK_NULL_HANDLE;
        last_sampler = VK_NULL_HANDLE;
    }
    docking_sampler_.reset();
    interface_.reset();
}