//
// Created by Gianni on 2/01/2025.
//

#include "instance.hpp"

void VulkanInstance::create()
{
    std::vector<const char*> extensions = instanceExtensions();

    VkApplicationInfo applicationInfo {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "Vulkan3DModelViewer",
        .applicationVersion = VK_MAKE_API_VERSION(0, 1, 0, 0),
        .pEngineName = nullptr,
        .apiVersion = VK_API_VERSION_1_1
    };

    VkInstanceCreateInfo instanceCreateInfo {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &applicationInfo,
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data()
    };

#ifdef DEBUG_MODE
    VkDebugUtilsMessageSeverityFlagsEXT debugUtilsMessageSeverityFlags {
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT
    };

    VkDebugUtilsMessageTypeFlagsEXT debugUtilsMessageTypeFlags {
        VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT
    };

    VkDebugUtilsMessengerCreateInfoEXT debugUtilsMessengerCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = debugUtilsMessageSeverityFlags,
        .messageType = debugUtilsMessageTypeFlags,
        .pfnUserCallback = debugCallback,
        .pUserData = nullptr
    };

    const char* validationLayer = "VK_LAYER_KHRONOS_validation";
    instanceCreateInfo.enabledLayerCount = 1;
    instanceCreateInfo.ppEnabledLayerNames = &validationLayer;
    instanceCreateInfo.pNext = &debugUtilsMessengerCreateInfo;
#endif

    // create vulkan instance
    VkResult result = vkCreateInstance(&instanceCreateInfo, nullptr, &instance);
    vulkanCheck(result, "Failed to create Vulkan instance");

#ifdef DEBUG_MODE
    // create the debug messenger
    auto funcPtr = vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    auto vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(funcPtr);

    result = vkCreateDebugUtilsMessengerEXT(instance,
                                            &debugUtilsMessengerCreateInfo,
                                            nullptr,
                                            &debugMessenger);
    vulkanCheck(result, "Failed to create debug messenger");
#endif

    // create surface
    result = glfwCreateWindowSurface(instance, glfwGetCurrentContext(), nullptr, &surface);
    vulkanCheck(result, "Failed to create surface");
}

void VulkanInstance::destroy()
{
#ifdef DEBUG_MODE
    auto funcPtr = vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    auto vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(funcPtr);
    vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
#endif
    vkDestroySurfaceKHR(instance, surface, nullptr);
    vkDestroyInstance(instance, nullptr);
}

std::vector<const char *> VulkanInstance::instanceExtensions()
{
    std::vector<const char*> extensions {
        "VK_KHR_surface",
        "VK_KHR_win32_surface"
    };

#ifdef DEBUG_MODE
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif

    return extensions;
}

VkBool32 VKAPI_CALL VulkanInstance::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                                  VkDebugUtilsMessageTypeFlagsEXT type,
                                                  const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                                  void *userPointer)
{
    std::cerr << "----- Validation Layer -----" << '\n';

    std::cerr << "Severity: ";
    switch (severity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: std::cerr << "VERBOSE"; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: std::cerr << "INFO"; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: std::cerr << "WARNING"; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: std::cerr << "ERROR"; break;
        default: std::cerr << "debugCallback: Uncovered code path reached";
    }

    std::cerr << "\nMessage Type: ";
    switch (type)
    {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: std::cerr << "GENERAL"; break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: std::cerr << "VALIDATION"; break;
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: std::cerr << "PERFORMANCE"; break;
        default: std::cerr << "debugCallback: Uncovered code path reached";
    }

    std::cerr << "\nMessage: " << pCallbackData->pMessage;
    std::cerr << "\n----------------------------\n\n";

    __debugbreak();

    return VK_FALSE;
}
