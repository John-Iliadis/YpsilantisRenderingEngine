//
// Created by Gianni on 2/01/2025.
//

#include "vulkan_device.hpp"

////////////////////////////////////////////////////////////////////////////////////
/// Vulkan Physical Device
////////////////////////////////////////////////////////////////////////////////////

void VulkanPhysicalDevice::create(const VulkanInstance& instance)
{
    pickPhysicalDevice(instance);
    vkGetPhysicalDeviceProperties(physicalDevice, &properties);
    vkGetPhysicalDeviceFeatures(physicalDevice, &features);
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
    findQueueFamilyIndices();
}

void VulkanPhysicalDevice::pickPhysicalDevice(const VulkanInstance& instance)
{
    uint32_t physicalDeviceCount;
    vkEnumeratePhysicalDevices(instance.instance, &physicalDeviceCount, nullptr);

    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(instance.instance, &physicalDeviceCount, physicalDevices.data());

    bool deviceFound = false;
    for (VkPhysicalDevice physicalDevice : physicalDevices)
    {
        VkPhysicalDeviceProperties properties_;
        vkGetPhysicalDeviceProperties(physicalDevice, &properties_);

        if (properties_.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            this->physicalDevice = physicalDevice;
            deviceFound = true;
            break;
        }
    }

    if (!deviceFound)
    {
        debugLog("Discrete GPU not found.");
        physicalDevice = physicalDevices.at(0);
    }
}

void VulkanPhysicalDevice::findQueueFamilyIndices()
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


////////////////////////////////////////////////////////////////////////////////////
/// Vulkan Logical Device
////////////////////////////////////////////////////////////////////////////////////

void VulkanDevice::create(const VulkanPhysicalDevice &physicalDevice)
{
    std::vector<const char*> extensions {
        "VK_KHR_swapchain",
        "VK_EXT_descriptor_indexing"
    };

    VkPhysicalDeviceDescriptorIndexingFeaturesEXT physicalDeviceDescriptorIndexingFeatures {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT,
        .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
        .descriptorBindingVariableDescriptorCount = VK_TRUE,
        .runtimeDescriptorArray = VK_TRUE
    };

    float queuePriority = 1.f;
    VkDeviceQueueCreateInfo queueCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = physicalDevice.graphicsQueueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };

    VkDeviceCreateInfo deviceCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &physicalDeviceDescriptorIndexingFeatures,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
        .pEnabledFeatures = nullptr
    };

    VkResult result = vkCreateDevice(physicalDevice.physicalDevice, &deviceCreateInfo, nullptr, &device);
    vulkanCheck(result, "Failed to create logical device.");

    vkGetDeviceQueue(device, physicalDevice.graphicsQueueFamilyIndex, 0, &graphicsQueue);
}

void VulkanDevice::destroy()
{
    vkDestroyDevice(device, nullptr);
}
