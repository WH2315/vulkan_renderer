#include "application.hpp"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

static Application* application = nullptr;

static ImGui_ImplVulkanH_Window imgui_vk_window;
static uint32_t min_image_count = 2;
static bool swapchain_rebuild = false;

static uint32_t current_frame_index = 0;

static std::vector<std::vector<std::function<void()>>> resource_free_queue;

static void frameRender(ImDrawData* data) {
    vk::Semaphore image_acquired_semaphore =
        imgui_vk_window.FrameSemaphores[imgui_vk_window.SemaphoreIndex]
            .ImageAcquiredSemaphore;
    vk::Semaphore render_complete_semaphore =
        imgui_vk_window.FrameSemaphores[imgui_vk_window.SemaphoreIndex]
            .RenderCompleteSemaphore;

    try {
        auto res = wen::manager->device->device.acquireNextImageKHR(
            imgui_vk_window.Swapchain, UINT64_MAX, image_acquired_semaphore, nullptr,
            &imgui_vk_window.FrameIndex);
        if (res == vk::Result::eErrorOutOfDateKHR ||
            res == vk::Result::eSuboptimalKHR) {
            swapchain_rebuild = true;
            return;
        }
    } catch (vk::OutOfDateKHRError) {
        swapchain_rebuild = true;
        return;
    }

    ImGui_ImplVulkanH_Frame* frame =
        &imgui_vk_window.Frames[imgui_vk_window.FrameIndex];
    auto result =
        wen::manager->device->device.waitForFences({frame->Fence}, true, UINT64_MAX);
    if (result != vk::Result::eSuccess) {
        WEN_ERROR("Failed to wait for fence")
    }

    wen::manager->device->device.resetFences({frame->Fence});

    current_frame_index = (current_frame_index + 1) % imgui_vk_window.ImageCount;

    for (auto& func : resource_free_queue[current_frame_index]) {
        func();
    }
    resource_free_queue[current_frame_index].clear();

    wen::manager->device->device.resetCommandPool(frame->CommandPool);

    vk::CommandBuffer cmdbuf = frame->CommandBuffer;
    vk::CommandBufferBeginInfo begin = {};
    begin.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    cmdbuf.begin(begin);
    vk::RenderPassBeginInfo render_pass_begin = {};
    vk::ClearValue clear_values;
    clear_values.color.float32[0] = imgui_vk_window.ClearValue.color.float32[0];
    clear_values.color.float32[1] = imgui_vk_window.ClearValue.color.float32[1];
    clear_values.color.float32[2] = imgui_vk_window.ClearValue.color.float32[2];
    clear_values.color.float32[3] = imgui_vk_window.ClearValue.color.float32[3];
    render_pass_begin.setRenderPass(imgui_vk_window.RenderPass)
        .setFramebuffer(frame->Framebuffer)
        .setRenderArea({
            {                              0,                                0},
            {(uint32_t)imgui_vk_window.Width, (uint32_t)imgui_vk_window.Height}
    })
        .setClearValueCount(1)
        .setClearValues(clear_values);
    cmdbuf.beginRenderPass(render_pass_begin, vk::SubpassContents::eInline);
    ImGui_ImplVulkan_RenderDrawData(data, frame->CommandBuffer);
    cmdbuf.endRenderPass();
    cmdbuf.end();

    vk::PipelineStageFlags wait_stage =
        vk::PipelineStageFlagBits::eColorAttachmentOutput;
    vk::SubmitInfo submits = {};
    submits.setWaitSemaphores(image_acquired_semaphore)
        .setWaitDstStageMask(wait_stage)
        .setCommandBuffers(cmdbuf)
        .setSignalSemaphores(render_complete_semaphore);
    wen::manager->device->graphics_queue.submit(submits, frame->Fence);
}

static void framePresent() {
    if (swapchain_rebuild) {
        return;
    }

    vk::Semaphore render_complete_semaphore =
        imgui_vk_window.FrameSemaphores[imgui_vk_window.SemaphoreIndex]
            .RenderCompleteSemaphore;
    vk::PresentInfoKHR present = {};
    vk::SwapchainKHR swapchain = imgui_vk_window.Swapchain;
    present.setWaitSemaphores(render_complete_semaphore)
        .setSwapchains(swapchain)
        .setImageIndices(imgui_vk_window.FrameIndex)
        .setPResults(nullptr);

    try {
        auto res = wen::manager->device->graphics_queue.presentKHR(present);
        if (res == vk::Result::eErrorOutOfDateKHR ||
            res == vk::Result::eSuboptimalKHR) {
            swapchain_rebuild = true;
            return;
        }
    } catch (vk::OutOfDateKHRError) {
        swapchain_rebuild = true;
        return;
    }

    imgui_vk_window.SemaphoreIndex =
        (imgui_vk_window.SemaphoreIndex + 1) % imgui_vk_window.ImageCount;
}

Application::Application() {
    window_ = wen::g_window->getWindow();
}

void Application::init() {
    vk::DescriptorPoolSize sizes[] = {
        {             vk::DescriptorType::eSampler, 1000},
        {vk::DescriptorType::eCombinedImageSampler, 1000},
        {        vk::DescriptorType::eSampledImage, 1000},
        {        vk::DescriptorType::eStorageImage, 1000},
        {  vk::DescriptorType::eUniformTexelBuffer, 1000},
        {  vk::DescriptorType::eStorageTexelBuffer, 1000},
        {       vk::DescriptorType::eUniformBuffer, 1000},
        {       vk::DescriptorType::eStorageBuffer, 1000},
        {vk::DescriptorType::eUniformBufferDynamic, 1000},
        {vk::DescriptorType::eStorageBufferDynamic, 1000},
        {     vk::DescriptorType::eInputAttachment, 1000}
    };
    vk::DescriptorPoolCreateInfo ci = {};
    ci.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
        .setMaxSets(1000 * IM_ARRAYSIZE(sizes))
        .setPoolSizeCount(static_cast<uint32_t>(IM_ARRAYSIZE(sizes)))
        .setPoolSizes(sizes);
    descriptor_pool_ = wen::manager->device->device.createDescriptorPool(ci);
    glfwCreateWindowSurface(wen::manager->vk_instance, window_, nullptr,
                            &imgui_vk_window.Surface);
    const vk::Format formats[] = {vk::Format::eB8G8R8A8Unorm,
                                  vk::Format::eR8G8B8A8Unorm, vk::Format::eB8G8R8Unorm,
                                  vk::Format::eR8G8B8Unorm};
    imgui_vk_window.SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(
        wen::manager->device->physical_device, imgui_vk_window.Surface,
        (VkFormat*)formats, 4, (VkColorSpaceKHR)(vk::ColorSpaceKHR::eSrgbNonlinear));
    VkPresentModeKHR present_modes[] = {VK_PRESENT_MODE_FIFO_KHR};
    imgui_vk_window.PresentMode = ImGui_ImplVulkanH_SelectPresentMode(
        wen::manager->device->physical_device, imgui_vk_window.Surface,
        &present_modes[0], 1);
    int width, height;
    glfwGetFramebufferSize(window_, &width, &height);
    ImGui_ImplVulkanH_CreateOrResizeWindow(
        wen::manager->vk_instance, wen::manager->device->physical_device,
        wen::manager->device->device, &imgui_vk_window,
        wen::manager->device->graphics_queue_family, nullptr,

        width, height, min_image_count, 0);
    resource_free_queue.resize(imgui_vk_window.ImageCount);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    ImGui::StyleColorsClassic();
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    ImGui_ImplGlfw_InitForVulkan(window_, true);
    ImGui_ImplVulkan_InitInfo info = {};
    info.Instance = wen::manager->vk_instance;
    info.PhysicalDevice = wen::manager->device->physical_device;
    info.Device = wen::manager->device->device;
    info.QueueFamily = wen::manager->device->graphics_queue_family;
    info.Queue = wen::manager->device->graphics_queue;
    info.PipelineInfoMain.RenderPass = imgui_vk_window.RenderPass;
    info.PipelineCache = nullptr;
    info.DescriptorPool = descriptor_pool_;
    info.PipelineInfoMain.Subpass = 0;
    info.MinImageCount = min_image_count;
    info.ImageCount = imgui_vk_window.ImageCount;
    info.PipelineInfoMain.MSAASamples =
        (VkSampleCountFlagBits)vk::SampleCountFlagBits::e1;
    info.Allocator = nullptr;
    info.CheckVkResultFn = [](VkResult res) {
        if (res != VK_SUCCESS) {
            fprintf(stderr, "[vulkan] Error: VkResult = %d\n", res);
        }
    };
    ImGui_ImplVulkan_Init(&info);
    ImFontConfig config;
    io.Fonts->AddFontFromFileTTF("resources/JetBrainsMonoNLNerdFontMono-Bold.ttf",
                                 18.0f, &config, io.Fonts->GetGlyphRangesDefault());
}

void Application::run() {
    application = this;
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
        for (auto& layer : layers_) {
            layer->update(ImGui::GetIO().DeltaTime);
        }
        if (swapchain_rebuild) {
            int width, height;
            glfwGetFramebufferSize(window_, &width, &height);
            if (width > 0 && height > 0) {
                ImGui_ImplVulkan_SetMinImageCount(min_image_count);
                ImGui_ImplVulkanH_CreateOrResizeWindow(
                    wen::manager->vk_instance, wen::manager->device->physical_device,
                    wen::manager->device->device, &imgui_vk_window,
                    wen::manager->device->graphics_queue_family, nullptr, width, height,
                    min_image_count, 0);
                imgui_vk_window.FrameIndex = 0;
                swapchain_rebuild = false;
            }
        }
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                        ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                        ImGuiWindowFlags_NoBringToFrontOnFocus |
                        ImGuiWindowFlags_NoNavFocus;
        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode) {
            window_flags |= ImGuiWindowFlags_NoBackground;
        }
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("DockSpace Demo", nullptr, window_flags);
        ImGui::PopStyleVar();
        ImGui::PopStyleVar(2);
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
            ImGuiID dockspace_id = ImGui::GetID("VulkanAppDockspace");
            ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
        }
        for (auto& layer : layers_) {
            layer->render();
        }
        ImGui::End();
        ImGui::Render();
        ImDrawData* data = ImGui::GetDrawData();
        const bool minimized =
            (data->DisplaySize.x <= 0.0f || data->DisplaySize.y <= 0.0f);
        ImVec4 clearColor = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);
        imgui_vk_window.ClearValue.color.float32[0] = clearColor.x * clearColor.w;
        imgui_vk_window.ClearValue.color.float32[1] = clearColor.y * clearColor.w;
        imgui_vk_window.ClearValue.color.float32[2] = clearColor.z * clearColor.w;
        imgui_vk_window.ClearValue.color.float32[3] = clearColor.w;

        if (!minimized) {
            frameRender(data);
        }

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }

        if (!minimized) {
            framePresent();
        }
    }
}

Application::~Application() {
    application = nullptr;
    layers_.clear();

    wen::manager->device->device.waitIdle();
    for (auto& queue : resource_free_queue) {
        for (auto& func : queue) {
            func();
        }
    }
    resource_free_queue.clear();

    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    ImGui_ImplVulkanH_DestroyWindow(wen::manager->vk_instance,
                                    wen::manager->device->device, &imgui_vk_window,
                                    nullptr);

    wen::manager->device->device.destroyDescriptorPool(descriptor_pool_);
}

Application& Application::get() {
    return *application;
}

vk::PhysicalDevice Application::getPhysicalDevice() {
    return wen::manager->device->physical_device;
}

vk::Device Application::getDevice() {
    return wen::manager->device->device;
}

vk::CommandBuffer Application::allocateSingleUse() {
    ImGui_ImplVulkanH_Window* wd = &imgui_vk_window;
    vk::CommandPool command_pool = wd->Frames[wd->FrameIndex].CommandPool;

    vk::CommandBufferAllocateInfo ai = {};
    ai.setCommandPool(command_pool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        .setCommandBufferCount(1);

    auto cmdbuf = wen::manager->device->device.allocateCommandBuffers(ai)[0];
    vk::CommandBufferBeginInfo begin = {};
    begin.setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);
    cmdbuf.begin(begin);

    return cmdbuf;
}

void Application::freeSingleUse(vk::CommandBuffer cmdbuf) {
    cmdbuf.end();

    ImGui_ImplVulkanH_Window* wd = &imgui_vk_window;
    vk::CommandPool command_pool = wd->Frames[wd->FrameIndex].CommandPool;

    vk::SubmitInfo submits = {};
    submits.setCommandBuffers(cmdbuf);
    wen::manager->device->graphics_queue.submit(submits, nullptr);
    wen::manager->device->graphics_queue.waitIdle();
    wen::manager->device->device.freeCommandBuffers(command_pool, cmdbuf);
}

void Application::submitResourceFree(std::function<void()>&& func) {
    resource_free_queue[current_frame_index].emplace_back(func);
}