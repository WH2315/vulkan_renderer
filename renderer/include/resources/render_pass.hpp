#pragma once

#include "resources/render_subpass.hpp"
#include "base/enums.hpp"
#include <map>

namespace wen {

struct AttachmentInfo {
    vk::AttachmentDescription write_attachment = {};
    vk::ImageUsageFlags write_usage = {};
    vk::ImageAspectFlags aspect = {};
    vk::ClearValue clear_color = {};
};

class RenderPass {
public:
    RenderPass();
    ~RenderPass();

    void addAttachment(const std::string& name, AttachmentType type);
    RenderSubpass& addSubpass(const std::string& name);
    void addSubpassDependency(const std::string& src, const std::string& dst, std::array<vk::PipelineStageFlags, 2> stage, std::array<vk::AccessFlags, 2> access);

    void build();
    void update();

    uint32_t getAttachmentIndex(const std::string& name) const;
    uint32_t getSubpassIndex(const std::string& name) const;

public:
    std::vector<AttachmentInfo> attachments;
    std::vector<std::unique_ptr<RenderSubpass>> subpasses;

    vk::RenderPass render_pass;
    std::vector<vk::AttachmentDescription> final_attachments;
    std::vector<vk::SubpassDescription> final_subpasses;
    std::vector<vk::SubpassDependency> final_dependencies;

private:
    std::map<std::string, uint32_t> attachment_indices_;
    std::map<std::string, uint32_t> subpass_indices_;
};

} // namespace wen