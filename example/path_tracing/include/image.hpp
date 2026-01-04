#pragma once

#include <vulkan/vulkan.hpp>

enum class ImageFormat { None = 0, RGBA, RGBA32F };

class Image {
public:
    Image(uint32_t width, uint32_t height, ImageFormat format);
    ~Image();

    void create();
    void set(const void* data);
    void resize(uint32_t width, uint32_t height);
    void reset();

    VkDescriptorSet id() const { return descriptor_set_; }

    uint32_t width() const { return width_; }

    uint32_t height() const { return height_; }

    uint8_t* data() const { return data_; }

private:
    uint8_t* data_ = nullptr;
    ImageFormat format_ = ImageFormat::None;

    uint32_t width_ = 0, height_ = 0;
    vk::Image image_ = nullptr;
    vk::DeviceMemory image_memory_ = nullptr;
    vk::ImageView image_view_ = nullptr;
    vk::Sampler sampler_ = nullptr;
    VkDescriptorSet descriptor_set_ = nullptr;

    vk::Buffer staging_ = nullptr;
    vk::DeviceMemory staging_memory_ = nullptr;
    vk::Buffer buffer_ = nullptr;
    vk::DeviceMemory buffer_memory_ = nullptr;
};