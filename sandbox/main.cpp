#include "wen.hpp"
#include <glm/glm.hpp>
#include <iostream>

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

    auto vert_shader = interface->loadShader("shader.vert", wen::ShaderStage::eVertex);
    auto frag_shader = interface->loadShader("shader.frag", wen::ShaderStage::eFragment);
    auto shader_program = interface->createShaderProgram();
    shader_program->attach(vert_shader).attach(frag_shader);

    struct Vertex {
        glm::vec3 position;
        glm::vec3 color;
    };
    float rad = 3.1415926f / 180.0f;
    float R = 1;
    float r = R * sin(18 * rad) / cos(36 * rad);
    glm::vec2 RVertex[5];
    glm::vec2 rVertex[5];
    for (int k = 0; k < 5; k++) {
       RVertex[k] = {-(R * cos((90 + k * 72 + 18.0f) * rad)), -(R * sin((90 + k * 72 + 18.0f) * rad))};
       rVertex[k] = {-(r * cos((90 + 36 + k * 72 + 18.0f) * rad)), -(r * sin((90 + 36 + k * 72 + 18.0f) * rad))};
    }
    std::vector<Vertex> vertices = {
        {{0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}},
    };
    for (int i = 0; i < 5; i++) {
        vertices.push_back({{RVertex[i], 0.0f}, {1.0f, 0.0f, 0.0f}});
        vertices.push_back({{rVertex[i], 0.0f}, {1.0f, 0.0f, 0.0f}});
    }
    std::vector<uint16_t> indices;
    for (int i = 1; i <= 10; i++) {
        indices.push_back(0);
        indices.push_back(i);
        indices.push_back(i % 10 + 1);
    }

    auto vertex_input = interface->createVertexInput({
        {
            .binding = 0,
            .input_rate = wen::InputRate::eVertex,
            .formats = {
                wen::VertexType::eFloat3, // position
                wen::VertexType::eFloat3  // color
            }
        }
    });

    auto vertex_buffer = interface->createVertexBuffer(sizeof(Vertex), vertices.size());
    vertex_buffer->setData(vertices);
    auto index_buffer = interface->createIndexBuffer(wen::IndexType::eUint16, indices.size());
    index_buffer->setData(indices);

    auto render_pipeline = interface->createRenderPipeline(renderer, shader_program, "main_subpass");
    render_pipeline->setVertexInput(vertex_input);
    render_pipeline->compile({
        .polygon_mode = vk::PolygonMode::eLine,
        .line_width = 5.0f,
        .depth_test_enable = false,
        .dynamic_states = {
            vk::DynamicState::eViewport,
            vk::DynamicState::eScissor
        }
    });

    while (!manager->shouldClose()) {
        manager->pollEvents();

        auto width = wen::renderer_config->getWidth(), height = wen::renderer_config->getHeight();
        auto w = static_cast<float>(width), h = static_cast<float>(height);

        renderer->setClearColor("swapchain_image", {{0.5f, 0.5f, 0.5f, 1.0f}});

        renderer->beginRender();
        renderer->bindPipeline(render_pipeline);
        renderer->setViewport(0, h, w, -h);
        renderer->setScissor(0, 0, width, height);
        renderer->bindVertexBuffer(vertex_buffer);
        renderer->bindIndexBuffer(index_buffer); 
        // renderer->draw(3, 1, 0, 0);
        renderer->drawIndexed(indices.size(), 1, 0, 0, 0);
        renderer->endRender();
    }
    renderer->waitIdle();

    render_pipeline.reset();
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