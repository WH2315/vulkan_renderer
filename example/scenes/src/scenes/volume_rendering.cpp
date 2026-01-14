#include "scenes/volume_rendering.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

void VolumeRendering::initialize() {
    auto render_pass = interface->createRenderPass(false);
    render_pass->addAttachment(wen::SWAPCHAIN_IMAGE_ATTACHMENT,
                               wen::AttachmentType::eColor);
    render_pass->addAttachment(wen::DEPTH_ATTACHMENT, wen::AttachmentType::eDepth);
    render_pass->addAttachment(wen::IMGUI_DOCKING_ATTACHMENT,
                               wen::AttachmentType::eRGBA8Snorm);

    auto& subpass = render_pass->addSubpass("main_subpass");
    subpass.setOutputAttachment(wen::IMGUI_DOCKING_ATTACHMENT);
    subpass.setDepthAttachment(wen::DEPTH_ATTACHMENT);

    render_pass->addSubpassDependency(
        wen::EXTERNAL_SUBPASS, "main_subpass",
        {vk::PipelineStageFlagBits::eColorAttachmentOutput |
             vk::PipelineStageFlagBits::eLateFragmentTests,
         vk::PipelineStageFlagBits::eColorAttachmentOutput |
             vk::PipelineStageFlagBits::eLateFragmentTests},
        {vk::AccessFlagBits::eColorAttachmentWrite |
             vk::AccessFlagBits::eDepthStencilAttachmentWrite,
         vk::AccessFlagBits::eColorAttachmentWrite |
             vk::AccessFlagBits::eDepthStencilAttachmentWrite});

    render_pass->build();

    renderer = interface->createRenderer(std::move(render_pass));
    imGui = std::make_shared<wen::Imgui>(*renderer, true);

    volume_data_args_ = interface->createUniformBuffer(sizeof(VolumeRenderingArgs));
    auto* ptr = static_cast<VolumeRenderingArgs*>(volume_data_args_->getData());
    *ptr = VolumeRenderingArgs{};

    volume_data_cloud_ =
        interface->loadVolumeData("wdas_cloud_density.match_volume_data");
    volume_buffer_cloud_ = interface->createVertexBuffer(
        sizeof(float), wen::VOLUME_RAW_DATA_RESOLUTION *
                           wen::VOLUME_RAW_DATA_RESOLUTION *
                           wen::VOLUME_RAW_DATA_RESOLUTION);
    volume_data_cloud_->uploadToBuffer(volume_buffer_cloud_);

    volume_data_smoke_ =
        interface->loadVolumeData("bunny_cloud_density.match_volume_data");
    volume_buffer_smoke_ = interface->createVertexBuffer(
        sizeof(float), wen::VOLUME_RAW_DATA_RESOLUTION *
                           wen::VOLUME_RAW_DATA_RESOLUTION *
                           wen::VOLUME_RAW_DATA_RESOLUTION);
    volume_data_smoke_->uploadToBuffer(volume_buffer_smoke_);

    sphere_ = interface->createSphereModel();
    sphere_->registerCustomSphereData<SphereType>();
}