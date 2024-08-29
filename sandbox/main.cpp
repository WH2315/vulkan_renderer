#include "wen.hpp"
#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "core/imgui.hpp"

int main() {
    wen::Manager* manager = new wen::Manager;

    manager->initializeEngine();

    wen::renderer_config->window_info = {"sandbox", 900, 900};
    wen::renderer_config->debug = true;
    wen::renderer_config->app_name = "sandbox";
    wen::renderer_config->engine_name = "wen";
    wen::renderer_config->sync = true;

    manager->initializeRenderer();

    auto interface = std::make_shared<wen::Interface>("sandbox/resources");

    auto render_pass = interface->createRenderPass();
    render_pass->addAttachment("swapchain_image", wen::AttachmentType::eColor);

    auto& subpass = render_pass->addSubpass("main_subpass");
    subpass.setOutputAttachment("swapchain_image");

    render_pass->addSubpassDependency(
        "external_subpass",
        "main_subpass",
        {
            vk::PipelineStageFlagBits::eColorAttachmentOutput,
            vk::PipelineStageFlagBits::eColorAttachmentOutput
        },
        {
            vk::AccessFlagBits::eNone,
            vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite
        }
    );

    render_pass->build();

    auto renderer = interface->createRenderer(std::move(render_pass));
    auto imgui = std::make_shared<wen::Imgui>(*renderer);

    auto vert_shader = interface->loadShader("shader.vert", wen::ShaderStage::eVertex);
    auto frag_shader = interface->loadShader("shader.frag", wen::ShaderStage::eFragment);
    auto shader_program = interface->createShaderProgram();
    shader_program->attach(vert_shader).attach(frag_shader);

    struct Vertex {
        glm::vec3 position;
        glm::vec3 color;
        glm::vec2 uv;
    };
    const std::vector<Vertex> vertices = {
        {{ 0.5f,  0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
        {{ 0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
        {{-0.5f,  0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
        {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
    };
    const std::vector<uint16_t> indices = {0, 1, 2, 1, 2, 3};

    auto vertex_input = interface->createVertexInput({
        {
            .binding = 0,
            .input_rate = wen::InputRate::eVertex,
            .formats = {
                wen::VertexType::eFloat3, // position
                wen::VertexType::eFloat3, // color
                wen::VertexType::eFloat2  // uv
            }
        }
    });

    auto vertex_buffer = interface->createVertexBuffer(sizeof(Vertex), vertices.size());
    vertex_buffer->setData(vertices);
    auto index_buffer = interface->createIndexBuffer(wen::IndexType::eUint16, indices.size());
    index_buffer->setData(indices);

    auto descriptor_set = interface->createDescriptorSet();
    descriptor_set->addDescriptors({
        {0, vk::DescriptorType::eUniformBuffer, wen::ShaderStage::eVertex},
        {1, vk::DescriptorType::eCombinedImageSampler, wen::ShaderStage::eFragment}
    });
    descriptor_set->build();

    auto render_pipeline = interface->createRenderPipeline(renderer, shader_program, "main_subpass");
    render_pipeline->setVertexInput(vertex_input);
    render_pipeline->setDescriptorSet(descriptor_set);
    render_pipeline->compile({
        .polygon_mode = vk::PolygonMode::eFill,
        .depth_test_enable = false,
        .dynamic_states = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        }
    });

    struct UBO {
        glm::mat4 model;
    } ubo;

    auto uniform_buffer = interface->createUniformBuffer(sizeof(UBO));

    auto texture = interface->createTexture("texture.jpg");
    auto sampler = interface->createSampler({
        .mag_filter = vk::Filter::eNearest,
        .min_filter = vk::Filter::eLinear,
        .address_mode_u = vk::SamplerAddressMode::eMirroredRepeat,
        .address_mode_v = vk::SamplerAddressMode::eMirroredRepeat,
        .address_mode_w = vk::SamplerAddressMode::eRepeat,
        .max_anisotropy = 16,
        .border_color = vk::BorderColor::eFloatOpaqueBlack,
        .mipmap_mode = vk::SamplerMipmapMode::eLinear,
        .mip_levels = texture->getMipLevels()
    });

    descriptor_set->bindUniform(0, uniform_buffer);
    descriptor_set->bindTexture(1, texture, sampler);

    while (!manager->shouldClose()) {
        manager->pollEvents();

        static auto start = std::chrono::high_resolution_clock::now();
        auto current = std::chrono::high_resolution_clock::now();
        auto time = std::chrono::duration<float, std::chrono::seconds::period>(current - start).count();

        auto width = wen::renderer_config->getWidth(), height = wen::renderer_config->getHeight();
        auto w = static_cast<float>(width), h = static_cast<float>(height);

        renderer->setClearColor("swapchain_image", {{0.5f, 0.5f, 0.5f, 1.0f}});

        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        memcpy(uniform_buffer->getData(), &ubo, sizeof(UBO));

        renderer->beginRender();
        renderer->bindPipeline(render_pipeline);
        renderer->bindDescriptorSets(render_pipeline);
        renderer->setViewport(0, h, w, -h);
        renderer->setScissor(0, 0, width, height);
        renderer->bindVertexBuffer(vertex_buffer);
        renderer->bindIndexBuffer(index_buffer); 
        renderer->drawIndexed(indices.size(), 1, 0, 0, 0);

        imgui->begin();
        ImGui::Text("(%.1f FPS)", ImGui::GetIO().Framerate);
        imgui->end();

        renderer->endRender();
    }
    renderer->waitIdle();

    imgui.reset();
    sampler.reset();
    texture.reset();
    uniform_buffer.reset();
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