#include "context.hpp"
#include "core/log.hpp"
#include "base/configuration.hpp"
#include <map>

namespace wen {

std::unique_ptr<Context> Context::instance_ = nullptr;

void Context::init() {
    if (!instance_.get()) {
        instance_.reset(new Context);
    }
}

Context& Context::instance() {
    return *instance_;
}

void Context::quit() {
    instance_.reset(nullptr);
}

void Context::initialize() {
    createVkInstance();
    createSurface();
    device = std::make_unique<Device>();
    WEN_INFO("Vulkan Context Initialized!")
}

void Context::createVkInstance() {
    vk::ApplicationInfo app_info;
    app_info.setPApplicationName(renderer_config->app_name.c_str())
        .setApplicationVersion(1)
        .setPEngineName(renderer_config->engine_name.c_str())
        .setEngineVersion(1)
        .setApiVersion(vk::ApiVersion13);
    
    vk::InstanceCreateInfo create_info;
    create_info.setPApplicationInfo(&app_info);

    std::map<std::string, bool> requiredExtensions;
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    for (uint32_t i = 0; i < glfwExtensionCount; i++) {
        requiredExtensions.insert(std::make_pair(static_cast<std::string>(glfwExtensions[i]), false));
    }

    std::vector<const char*> extensions;
    auto EX = vk::enumerateInstanceExtensionProperties();
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
        auto LA = vk::enumerateInstanceLayerProperties();
        bool found0 = false, found1 = false;
        for (const auto& la : LA) {
            if (strcmp(la.layerName, layers[0]) == 0) {
                found0 = true;
            }
        }
        for (const auto& la : LA) {
            if (strcmp(la.layerName, layers[1]) == 0) {
                found1 = true;
            }
        }
        if (found0 && found1) {
            create_info.setPEnabledLayerNames(layers);
        } else {
            WEN_ERROR("Validation layer or/and Monitor layer not support!")
        }
    }

    vk_instance = vk::createInstance(create_info);
}

void Context::createSurface() {
    VkSurfaceKHR _surface;
    auto result = glfwCreateWindowSurface(static_cast<VkInstance>(vk_instance), g_window->getWindow(), nullptr, &_surface);
    if (result != VK_SUCCESS) {
        WEN_ERROR("Failed to create window surface!")
    }
    surface = vk::SurfaceKHR(_surface);
}

void Context::destroy() {
    device.reset();
    vk_instance.destroySurfaceKHR(surface);
    vk_instance.destroy();
    WEN_INFO("Vulkan Context Destroyed!")
}

} // namespace wen