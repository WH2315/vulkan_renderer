#pragma once

#include "resources/render_pass.hpp"
#include "resources/render_pipeline.hpp"
#include "resources/vertex_input/vertex_buffer.hpp"
#include "resources/vertex_input/index_buffer.hpp"
#include "resources/framebuffer_set.hpp"

namespace wen {

class Renderer {
public:
    Renderer(std::shared_ptr<RenderPass> render_pass);
    ~Renderer();

    void acquireNextImage();
    void beginRenderPass();
    void beginRender();
    void endRenderPass(); 
    void present();
    void endRender();

public:
    void setClearColor(const std::string& name, const vk::ClearValue& value);

public:
    void bindPipeline(const std::shared_ptr<RenderPipeline>& render_pipeline);
    void bindDescriptorSets(const std::shared_ptr<RenderPipeline>& render_pipeline);
    void pushConstants(const std::shared_ptr<RenderPipeline>& render_pipeline);
    void setViewport(float x, float y, float width, float height);
    void setScissor(int x, int y, uint32_t width, uint32_t height);
    void bindVertexBuffers(const std::vector<std::shared_ptr<VertexBuffer>>& vertex_buffers, uint32_t first_binding = 0);
    void bindVertexBuffer(const std::shared_ptr<VertexBuffer>& vertex_buffer, uint32_t binding = 0);
    void bindIndexBuffer(const std::shared_ptr<IndexBuffer>& index_buffer);
    void draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);
    void drawIndexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, uint32_t vertex_offset, uint32_t first_instance);
    void nextSubpass();
    void nextSubpass(const std::string& name);

public:
    vk::CommandBuffer getCurrentBuffer() { return current_buffer_; }
    void updateFramebuffers();
    void updateSwapchain();
    void updateRenderPass();
    void waitIdle();

public:
    std::shared_ptr<RenderPass> render_pass;
    std::unique_ptr<FramebufferSet> framebuffer_set;

private:
    uint32_t index_;

    uint32_t current_frame_ = 0;
    std::vector<vk::CommandBuffer> command_buffers_;
    vk::CommandBuffer current_buffer_;

    std::vector<vk::Semaphore> image_available_semaphores_;
    std::vector<vk::Semaphore> render_finished_semaphores_;
    std::vector<vk::Fence> in_flight_fences_;
    std::vector<std::vector<vk::SubmitInfo>> in_flight_submit_infos_;

    uint32_t current_subpass_;
};

} // namespace wen