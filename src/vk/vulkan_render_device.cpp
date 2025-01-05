//
// Created by Gianni on 2/01/2025.
//

#include "vulkan_render_device.hpp"

void VulkanRenderDevice::create(const VulkanInstance &instance)
{
    pickPhysicalDevice(instance);
    findQueueFamilyIndices();
    createLogicalDevice();
    createCommandPool();
}

void VulkanRenderDevice::destroy()
{
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyDevice(device, nullptr);
}

void VulkanRenderDevice::pickPhysicalDevice(const VulkanInstance &instance)
{
    uint32_t physicalDeviceCount;
    vkEnumeratePhysicalDevices(instance.instance, &physicalDeviceCount, nullptr);

    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(instance.instance, &physicalDeviceCount, physicalDevices.data());

    std::partition(physicalDevices.begin(), physicalDevices.end(),[] (VkPhysicalDevice physicalDevice) {
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(physicalDevice, &properties);
        return properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    });

    std::vector<const char*> requiredExtensions = getRequiredExtensions();

    bool deviceFound = false;
    for (VkPhysicalDevice physicalDevice : physicalDevices)
    {
        if (checkExtSupport(physicalDevice, requiredExtensions))
        {
            this->physicalDevice = physicalDevice;
            deviceFound = true;
            break;
        }
    }

    check(deviceFound, "Failed to find GPU that supports all required extensions.");

    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    debugLog(std::format("Selected GPU: {}.", properties.deviceName));
}

void VulkanRenderDevice::createLogicalDevice()
{
    std::vector<const char*> deviceExtensions = getRequiredExtensions();

    VkPhysicalDeviceDescriptorIndexingFeaturesEXT descriptorIndexingFeatures {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT,
        .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
        .descriptorBindingVariableDescriptorCount = VK_TRUE,
        .runtimeDescriptorArray = VK_TRUE
    };

    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeatures {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,
        .pNext = &descriptorIndexingFeatures,
        .extendedDynamicState = VK_TRUE
    };

    float queuePriority = 1.f;
    VkDeviceQueueCreateInfo queueCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = graphicsQueueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };

    VkPhysicalDeviceFeatures features {};

    VkDeviceCreateInfo deviceCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &extendedDynamicStateFeatures,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
        .ppEnabledExtensionNames = deviceExtensions.data(),
        .pEnabledFeatures = &features
    };

    VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);
    vulkanCheck(result, "Failed to create logical device.");

    vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);
}

void VulkanRenderDevice::findQueueFamilyIndices()
{
    uint32_t queueFamilyPropertyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyPropertyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyPropertyCount, queueFamilyProperties.data());

    for (uint32_t i = 0; i < queueFamilyProperties.size(); ++i)
    {
        if (queueFamilyProperties.at(i).queueFlags & VK_QUEUE_GRAPHICS_BIT)
        {
            graphicsQueueFamilyIndex = i;
            break;
        }
    }
}

void VulkanRenderDevice::createCommandPool()
{
    VkCommandPoolCreateInfo commandPoolCreateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = graphicsQueueFamilyIndex
    };

    VkResult result = vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool);
    vulkanCheck(result, "Failed to create command pool.");
}

bool VulkanRenderDevice::checkExtSupport(VkPhysicalDevice physicalDevice, const char* extension)
{
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data());

    for (const auto& ext : extensions)
        if (std::string(ext.extensionName) == extension)
            return true;
    return false;
}

bool VulkanRenderDevice::checkExtSupport(VkPhysicalDevice physicalDevice, const std::vector<const char *> &extensions)
{
    for (auto ext : extensions)
        if (!checkExtSupport(physicalDevice, ext))
            return false;
    return true;
}

std::vector<const char *> VulkanRenderDevice::getRequiredExtensions()
{
    return {
        "VK_KHR_swapchain",
        "VK_EXT_descriptor_indexing",
        "VK_EXT_extended_dynamic_state"
    };
}
