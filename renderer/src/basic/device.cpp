#include "basic/device.hpp"
#include "manager.hpp"
#include <map>
#include <set>

namespace wen {

Device::Device() {
    auto devices = manager->vk_instance.enumeratePhysicalDevices();

    bool selected = false;
    for (auto device : devices) {
        if (suitable(device)) {
            selected = true;
            break;
        }
    }
    if (!selected) {
        WEN_ERROR("No suitable physical device found!")
    } else {
        auto properties = physical_device.getProperties();
        WEN_INFO("Selected device: {}", std::string(properties.deviceName))
        if (renderer_config->debug) {
            WEN_DEBUG(
                "  Device Driver Version: {}.{}.{}",
                VK_VERSION_MAJOR(properties.driverVersion),
                VK_VERSION_MINOR(properties.driverVersion),
                VK_VERSION_PATCH(properties.driverVersion)
            )
            WEN_DEBUG(
                "  Device API Version: {}.{}.{}",
                VK_VERSION_MAJOR(properties.apiVersion),
                VK_VERSION_MINOR(properties.apiVersion),
                VK_VERSION_PATCH(properties.apiVersion)
            )
            auto memory_properties = physical_device.getMemoryProperties();
            for (uint32_t i = 0; i < memory_properties.memoryHeapCount; i++) {
                float size = ((float)memory_properties.memoryHeaps[i].size) / 1024.0f / 1024.0f / 1024.0f;
                if (memory_properties.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal) {
                    WEN_DEBUG("  Local GPU Memory: {} Gib", size)
                } else {
                    WEN_DEBUG("  Shared GPU Memory: {} Gib", size)
                }
            }
        }
    }

    vk::DeviceCreateInfo create_info;

    vk::PhysicalDeviceFeatures features;
    features.setSamplerAnisotropy(true)
        .setDepthClamp(true)
        .setSampleRateShading(true)
        .setShaderInt64(true)
        .setFragmentStoresAndAtomics(true)
        .setMultiDrawIndirect(true)
        .setGeometryShader(true)
        .setFillModeNonSolid(true)
        .setWideLines(true);
    create_info.setPEnabledFeatures(&features);
    vk::PhysicalDeviceVulkan12Features features12;
    features12.setRuntimeDescriptorArray(true)
        .setBufferDeviceAddress(true)
        .setShaderSampledImageArrayNonUniformIndexing(true)
        .setDescriptorBindingVariableDescriptorCount(true)
        .setDrawIndirectCount(true)
        .setSamplerFilterMinmax(true);
    create_info.setPNext(&features12);

    std::map<std::string, bool> requiredExtensions = {
        {VK_KHR_SWAPCHAIN_EXTENSION_NAME, false},
        {VK_KHR_BIND_MEMORY_2_EXTENSION_NAME, false},
    };

    std::vector<const char*> extensions;
    auto EX = physical_device.enumerateDeviceExtensionProperties();
    for (const auto& ex : EX) {
        if (requiredExtensions.find(ex.extensionName) != requiredExtensions.end()) {
            requiredExtensions[ex.extensionName] = true;
            extensions.push_back(ex.extensionName);
        }
    }
    for (const auto& [ex, success] : requiredExtensions) {
        if (!success) {
            WEN_ERROR("unsupported extension: {0}", ex)
        }
    }
    create_info.setPEnabledExtensionNames(extensions);

    const char* layers[] = {"VK_LAYER_KHRONOS_validation", "VK_LAYER_LUNARG_monitor"};
    if (renderer_config->debug) {
        create_info.setPEnabledLayerNames(layers);
    }

    std::set<uint32_t> queue_family_indices = {
        graphics_queue_family,
        present_queue_family,
        transfer_queue_family,
        compute_queue_family
    };
    std::vector<vk::DeviceQueueCreateInfo> create_infos(queue_family_indices.size());
    uint32_t i = 0;
    float priorities = 1.0f;
    for (auto index : queue_family_indices) {
        create_infos[i].setQueueFamilyIndex(index)
            .setQueueCount(1)
            .setQueuePriorities(priorities);
        i++;
    }
    create_info.setQueueCreateInfos(create_infos);

    device = physical_device.createDevice(create_info);

    graphics_queue = device.getQueue(graphics_queue_family, 0);
    present_queue = device.getQueue(present_queue_family, 0);
    transfer_queue = device.getQueue(transfer_queue_family, 0);
    compute_queue = device.getQueue(compute_queue_family, 0);
}

bool Device::suitable(const vk::PhysicalDevice& device) {
    auto properties = device.getProperties();
    auto features = device.getFeatures();

    std::string device_name = properties.deviceName;
    if (properties.deviceType != vk::PhysicalDeviceType::eDiscreteGpu) {
        WEN_WARN("Device is not a discrete GPU: {}", device_name)
        return false;
    }

    if (!features.samplerAnisotropy) {
        WEN_WARN("Device does not support sampler anisotropy: {}", device_name)
        return false;
    }

    if (!features.depthClamp) {
        WEN_WARN("Device does not support depth clamp: {}", device_name)
        return false;
    }

    if (!features.sampleRateShading) {
        WEN_WARN("Device does not support sample rate shading: {}", device_name)
        return false;
    }

    if (!features.shaderInt64) {
        WEN_WARN("Device does not support shader int64: {}", device_name)
        return false;
    }

    auto queueFamilyProperties = device.getQueueFamilyProperties();
    uint32_t index = 0;
    bool found = false;
    auto flags = vk::QueueFlagBits::eGraphics|vk::QueueFlagBits::eTransfer|vk::QueueFlagBits::eCompute;
    for (auto property : queueFamilyProperties) {
        if ((property.queueFlags & flags) && device.getSurfaceSupportKHR(index, manager->surface)) {
            graphics_queue_family = index;
            present_queue_family = index;
            transfer_queue_family = index;
            compute_queue_family = index;
            found = true;
            break;
        }
        index++;
    }
    if (!found) {
        WEN_WARN("Device does not support required queue family: {}", std::string(properties.deviceName))
        return false;
    }

    physical_device = device;
    return true;
}

Device::~Device() {
    device.destroy();
}

} // namespace wen