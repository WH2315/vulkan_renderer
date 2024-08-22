#include "renderer.hpp"
#include "manager.hpp"

namespace wen {

Renderer::Renderer(std::shared_ptr<RenderPass> render_pass) {
    this->render_pass = render_pass;
    for (uint32_t i = 0; i < manager->swapchain->image_count; i++) {
        framebuffers.push_back(new Framebuffer(*this, manager->swapchain->image_views[i]));
    }

    current_frame_ = 0;
    command_buffers_ = manager->command_pool->allocateCommandBuffers(renderer_config->max_frames_in_flight);
    current_buffer_ = command_buffers_[0];

    image_available_semaphores_.resize(renderer_config->max_frames_in_flight);
    render_finished_semaphores_.resize(renderer_config->max_frames_in_flight);
    in_flight_fences_.resize(renderer_config->max_frames_in_flight);

    vk::SemaphoreCreateInfo semaphore;
    vk::FenceCreateInfo fence;
    fence.setFlags(vk::FenceCreateFlagBits::eSignaled);
    for (uint32_t i = 0; i < renderer_config->max_frames_in_flight; i++) {
        image_available_semaphores_[i] = manager->device->device.createSemaphore(semaphore);
        render_finished_semaphores_[i] = manager->device->device.createSemaphore(semaphore);
        in_flight_fences_[i] = manager->device->device.createFence(fence);
        in_flight_submit_infos_.emplace_back();
    }
}

Renderer::~Renderer() {
    waitIdle();
    for (uint32_t i = 0; i < renderer_config->max_frames_in_flight; i++) {
        manager->device->device.destroySemaphore(image_available_semaphores_[i]);
        manager->device->device.destroySemaphore(render_finished_semaphores_[i]);
        manager->device->device.destroyFence(in_flight_fences_[i]);
    }
    for (auto& framebuffer : framebuffers) {
        delete framebuffer;
    }
    framebuffers.clear();
    render_pass.reset();
}

void Renderer::updateFramebuffers() {
    for (auto& framebuffer : framebuffers) {
        delete framebuffer;
    }
    framebuffers.clear();
    for (uint32_t i = 0; i < manager->swapchain->image_count; i++) {
        framebuffers.push_back(new Framebuffer(*this, manager->swapchain->image_views[i]));
    }
}

void Renderer::updateSwapchain() {
    manager->recreateSwapchain();
    updateFramebuffers();
}

void Renderer::waitIdle() {
    manager->device->device.waitIdle();
}

void Renderer::acquireNextImage() {
    auto& device = manager->device->device;

    auto result =
        device.waitForFences(in_flight_fences_[current_frame_], VK_TRUE, UINT64_MAX);
    if (result != vk::Result::eSuccess) {
        WEN_ERROR("Failed to wait for fence")
    }

    try {
        if (device.acquireNextImageKHR(
                manager->swapchain->swapchain, std::numeric_limits<uint64_t>::max(),
                image_available_semaphores_[current_frame_], nullptr,
                &index_) == vk::Result::eErrorOutOfDateKHR) {
            updateSwapchain();
            return;
        }
    } catch (vk::OutOfDateKHRError) {
        updateSwapchain();
        return;
    }

    device.resetFences(in_flight_fences_[current_frame_]);

    current_buffer_.reset();
    vk::CommandBufferBeginInfo begin_info;
    current_buffer_.begin(begin_info);
}

void Renderer::beginRenderPass() {
    std::vector<vk::ClearValue> clear_values;
    clear_values.reserve(render_pass->attachments.size());
    for (const auto& attachment : render_pass->attachments) {
        clear_values.push_back(attachment.clear_color);
    }

    vk::RenderPassBeginInfo begin_info;
    auto w = renderer_config->getWidth(), h = renderer_config->getHeight();
    vk::Rect2D render_area{{0, 0}, {w, h}};
    begin_info.setRenderPass(render_pass->render_pass)
        .setFramebuffer(framebuffers[index_]->framebuffer_)
        .setRenderArea(render_area)
        .setClearValues(clear_values);
    current_buffer_.beginRenderPass(begin_info, vk::SubpassContents::eInline);
}

void Renderer::beginRender() {
    acquireNextImage();
    beginRenderPass(); 
}

void Renderer::endRenderPass() {
    current_buffer_.endRenderPass();
}

void Renderer::present() {
    current_buffer_.end();

    std::vector<vk::Semaphore> wait_semaphores = {
        image_available_semaphores_[current_frame_],
    };
    std::vector<vk::PipelineStageFlags> wait_stages = {
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
    };

    auto& submits = in_flight_submit_infos_[current_frame_].emplace_back();
    submits.setWaitSemaphores(wait_semaphores)
        .setWaitDstStageMask(wait_stages)
        .setCommandBuffers(current_buffer_)
        .setSignalSemaphores(render_finished_semaphores_[current_frame_]);
    manager->device->graphics_queue.submit(submits, in_flight_fences_[current_frame_]);

    vk::PresentInfoKHR present_info;
    present_info.setWaitSemaphores(render_finished_semaphores_[current_frame_])
        .setSwapchains(manager->swapchain->swapchain)
        .setImageIndices(index_)
        .setPResults(nullptr);

    try {
        auto result = manager->device->present_queue.presentKHR(present_info);
        if (result == vk::Result::eErrorOutOfDateKHR || result == vk::Result::eSuboptimalKHR) {
            updateSwapchain();
        }
    } catch (vk::OutOfDateKHRError) {
        updateSwapchain();
    }

    current_frame_ = (current_frame_ + 1) % renderer_config->max_frames_in_flight;
    current_buffer_ = command_buffers_[current_frame_];
    in_flight_submit_infos_[current_frame_].clear();
}

void Renderer::endRender() {
    endRenderPass();
    present();
}

void Renderer::setClearColor(const std::string& name, const vk::ClearValue& value) {
    uint32_t index = render_pass->getAttachmentIndex(name);
    render_pass->attachments[index].clear_color = value;
}

void Renderer::bindPipeline(const std::shared_ptr<RenderPipeline>& render_pipeline) {
    current_buffer_.bindPipeline(vk::PipelineBindPoint::eGraphics, render_pipeline->pipeline);
}

void Renderer::setViewport(float x, float y, float width, float height) {
    vk::Viewport viewport{x, y, width, height, 0.0f, 1.0f};
    current_buffer_.setViewport(0, {viewport});
}

void Renderer::setScissor(int x, int y, uint32_t width, uint32_t height) {
    vk::Rect2D scissor{{x, y}, {width, height}};
    current_buffer_.setScissor(0, {scissor});
}

void Renderer::bindVertexBuffers(const std::vector<std::shared_ptr<VertexBuffer>>& vertex_buffers, uint32_t first_binding) {
    std::vector<vk::Buffer> buffers;
    std::vector<vk::DeviceSize> offsets;
    for (const auto& vertex_buffer : vertex_buffers) {
        buffers.push_back(vertex_buffer->getBuffer());
        offsets.push_back(0);
    }
    current_buffer_.bindVertexBuffers(first_binding, buffers, offsets);
}

void Renderer::bindVertexBuffer(const std::shared_ptr<VertexBuffer>& vertex_buffer, uint32_t binding) {
    bindVertexBuffers({vertex_buffer}, binding);
}

void Renderer::bindIndexBuffer(const std::shared_ptr<IndexBuffer>& index_buffer) {
    current_buffer_.bindIndexBuffer(index_buffer->getBuffer(), 0, index_buffer->getIndexType());
}

void Renderer::draw(uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance) {
    current_buffer_.draw(vertex_count, instance_count, first_vertex, first_instance);
}

void Renderer::drawIndexed(uint32_t index_count, uint32_t instance_count, uint32_t first_index, uint32_t vertex_offset, uint32_t first_instance) {
    current_buffer_.drawIndexed(index_count, instance_count, first_index, vertex_offset, first_instance);
}

} // namespace wen