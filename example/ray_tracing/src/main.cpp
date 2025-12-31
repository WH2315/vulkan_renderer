#include "wen.hpp"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "core/imgui.hpp"
#include <backends/imgui_impl_vulkan.h>

int main() {
    wen::Manager* manager = new wen::Manager;

    manager->initializeEngine();

    wen::renderer_config->window_info = {"ray_tracing", 1600, 900};
    wen::renderer_config->debug = true;
    wen::renderer_config->app_name = "ray_tracing";
    wen::renderer_config->engine_name = "wen";
    wen::renderer_config->vsync = true;

    manager->initializeRenderer();

    auto interface = std::make_shared<wen::Interface>("example/ray_tracing/resources");

    // wen::renderer_config->setSampleCount(vk::SampleCountFlagBits::e64);

    auto render_pass = interface->createRenderPass(false);
    render_pass->addAttachment(wen::SWAPCHAIN_IMAGE_ATTACHMENT, wen::AttachmentType::eColor);
    render_pass->addAttachment(wen::DEPTH_ATTACHMENT, wen::AttachmentType::eDepth);
    render_pass->addAttachment(wen::IMGUI_DOCKING_ATTACHMENT, wen::AttachmentType::eRGBA8Snorm);

    auto& subpass = render_pass->addSubpass("main_subpass");
    subpass.setOutputAttachment(wen::IMGUI_DOCKING_ATTACHMENT);
    subpass.setDepthAttachment(wen::DEPTH_ATTACHMENT);

    render_pass->addSubpassDependency(
        wen::EXTERNAL_SUBPASS,
        "main_subpass",
        {
            vk::PipelineStageFlagBits::eColorAttachmentOutput|vk::PipelineStageFlagBits::eLateFragmentTests,
            vk::PipelineStageFlagBits::eColorAttachmentOutput|vk::PipelineStageFlagBits::eLateFragmentTests
        },
        {
            vk::AccessFlagBits::eColorAttachmentWrite|vk::AccessFlagBits::eDepthStencilAttachmentWrite,
            vk::AccessFlagBits::eColorAttachmentWrite|vk::AccessFlagBits::eDepthStencilAttachmentWrite
        }
    );

    render_pass->build();

    auto renderer = interface->createRenderer(std::move(render_pass));
    auto imgui = std::make_shared<wen::Imgui>(*renderer, true);

    auto vert_shader = interface->loadShader("shader.vert", wen::ShaderStage::eVertex);
    auto frag_shader = interface->loadShader("shader.frag", wen::ShaderStage::eFragment);
    auto shader_program = interface->createGraphicsShaderProgram();
    shader_program->attach(vert_shader).attach(frag_shader);

    struct Vertex {
        glm::vec3 pos;
        glm::vec2 uv;
    };
    const std::vector<Vertex> vertices = {
        {{-1.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
        {{ 1.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
        {{ 1.0f,  1.0f, 0.0f}, {1.0f, 1.0f}},
        {{-1.0f,  1.0f, 0.0f}, {0.0f, 1.0f}}
    };
    const std::vector<uint16_t> indices = {
        0, 1, 2, 2, 3, 0
    };

    auto vertex_input = interface->createVertexInput({
        {
            .binding = 0,
            .input_rate = wen::InputRate::eVertex,
            .formats = {
                wen::VertexType::eFloat3,
                wen::VertexType::eFloat2
            }
        }
    });

    auto vertex_buffer = interface->createVertexBuffer(sizeof(Vertex), vertices.size());
    vertex_buffer->setData(vertices);
    auto index_buffer = interface->createIndexBuffer(wen::IndexType::eUint16, indices.size());
    index_buffer->setData(indices);

    auto descriptor_set = interface->createDescriptorSet();
    descriptor_set->addDescriptors({
        {0, vk::DescriptorType::eCombinedImageSampler, wen::ShaderStage::eFragment}
    });
    descriptor_set->build();

    auto render_pipeline = interface->createGraphicsRenderPipeline(renderer, shader_program, "main_subpass");
    render_pipeline->setVertexInput(vertex_input);
    render_pipeline->setDescriptorSet(descriptor_set);
    render_pipeline->compile({
        .polygon_mode = vk::PolygonMode::eFill,
        .depth_test_enable = true,
        .dynamic_states = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        }
    });

    auto texture = interface->createTexture("texture.jpg");
    auto sampler = interface->createSampler({
        .mip_levels = texture->getMipLevels()
    });

    descriptor_set->bindTexture(0, texture, sampler);

    VkDescriptorSet image = VK_NULL_HANDLE;
    VkImageView last_view = VK_NULL_HANDLE;
    VkSampler last_sampler = VK_NULL_HANDLE;

    while (!manager->shouldClose()) {
        manager->pollEvents();

        static auto start = std::chrono::high_resolution_clock::now();
        auto current = std::chrono::high_resolution_clock::now();
        auto time = std::chrono::duration<float, std::chrono::seconds::period>(current - start).count();

        auto width = wen::renderer_config->getWidth(), height = wen::renderer_config->getHeight();
        auto w = static_cast<float>(width), h = static_cast<float>(height);

        VkImageView view =
            renderer->framebuffer_set->attachments
                .at(renderer->render_pass->getAttachmentIndex(
                    wen::IMGUI_DOCKING_ATTACHMENT, wen::renderer_config->msaa()))
                ->image_view;
        if (image == VK_NULL_HANDLE || view != last_view ||
            sampler->sampler != last_sampler) {
            if (image != VK_NULL_HANDLE) {
                ImGui_ImplVulkan_RemoveTexture(image);
            }
            image = ImGui_ImplVulkan_AddTexture(
                sampler->sampler, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            last_view = view;
            last_sampler = sampler->sampler;
        }

        renderer->acquireNextImage();
        renderer->beginRenderPass();

        renderer->bindPipeline(render_pipeline);
        renderer->bindDescriptorSets(render_pipeline);
        renderer->setViewport(0, h, w, -h);
        renderer->setScissor(0, 0, width, height);
        renderer->bindVertexBuffer(vertex_buffer);
        renderer->bindIndexBuffer(index_buffer);
        renderer->drawIndexed(indices.size(), 1, 0, 0, 0);

        imgui->newFrame();

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

        ImGui::Begin("Viewport");
        ImGui::Image(image, ImGui::GetContentRegionAvail());
        ImGui::End();

        ImGui::Begin("Settings");
        ImGui::Text("(%.1f FPS)", ImGui::GetIO().Framerate);
        ImGui::End();

        ImGui::End();

        imgui->renderFrame();

        renderer->endRenderPass();
        renderer->present();
    }
    renderer->waitIdle();

    imgui.reset();

    sampler.reset();
    texture.reset();
    render_pipeline.reset();
    descriptor_set.reset();
    index_buffer.reset();
    vertex_buffer.reset();
    vertex_input.reset();
    shader_program.reset();
    frag_shader.reset();
    vert_shader.reset();
    renderer.reset();

    interface.reset();

    manager->destroyRenderer();
    manager->destroyEngine();

    delete manager;

    return 0;
}