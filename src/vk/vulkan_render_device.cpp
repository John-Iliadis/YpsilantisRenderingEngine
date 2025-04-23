//
// Created by Gianni on 2/01/2025.
//

#include "vulkan_render_device.hpp"
#include "vulkan_utils.hpp"
#include "vulkan_descriptor.hpp"

static const std::vector<const char*> extensions {
    "VK_KHR_swapchain",
    "VK_EXT_extended_dynamic_state",
    "VK_EXT_host_query_reset",
    "VK_KHR_push_descriptor",
    "VK_EXT_descriptor_indexing"
};

static bool checkExtSupport(VkPhysicalDevice physicalDevice)
{
    std::unordered_set<const char*, cStrHash, cStrCompare> extensionsSet(extensions.begin(), extensions.end());

    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> supportedExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, supportedExtensions.data());

    for (const auto& ext : supportedExtensions)
        if (extensionsSet.contains(ext.extensionName))
            extensionsSet.erase(ext.extensionName);

    return extensionsSet.empty();
}

VulkanRenderDevice::VulkanRenderDevice(const VulkanInstance &instance)
{
    pickPhysicalDevice(instance);
    findQueueFamilyIndices();
    createLogicalDevice();
    createCommandPool();
    createDescriptorPool();
}

VulkanRenderDevice::~VulkanRenderDevice()
{
    vkDestroyCommandPool(device, commandPool, nullptr);
    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    vkDestroyDevice(device, nullptr);
}

void VulkanRenderDevice::pickPhysicalDevice(const VulkanInstance &instance)
{
    uint32_t physicalDeviceCount;
    vkEnumeratePhysicalDevices(instance.instance, &physicalDeviceCount, nullptr);

    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    vkEnumeratePhysicalDevices(instance.instance, &physicalDeviceCount, physicalDevices.data());

    std::partition(physicalDevices.begin(), physicalDevices.end(),[] (VkPhysicalDevice physicalDevice_) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice_, &deviceProperties);
        return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    });

    bool deviceFound = false;
    for (VkPhysicalDevice lPhysicalDevice : physicalDevices)
    {
        if (checkExtSupport(lPhysicalDevice))
        {
            physicalDevice = lPhysicalDevice;
            deviceFound = true;
            break;
        }
    }

    check(deviceFound, "Failed to find a GPU that supports all required extensions.");

    vkGetPhysicalDeviceProperties(physicalDevice, &mDeviceProperties);
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &mMemoryProperties);
}

void VulkanRenderDevice::createLogicalDevice()
{
    VkPhysicalDeviceHostQueryResetFeaturesEXT hostQueryResetFeatures {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_HOST_QUERY_RESET_FEATURES_EXT,
        .pNext = nullptr,
        .hostQueryReset = VK_TRUE
    };

    VkPhysicalDeviceExtendedDynamicStateFeaturesEXT extendedDynamicStateFeatures {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_EXTENDED_DYNAMIC_STATE_FEATURES_EXT,
        .pNext = &hostQueryResetFeatures,
        .extendedDynamicState = VK_TRUE
    };

    // note: can still get maxPerStageDescriptorSamplers limit exceeded error when using this
    VkPhysicalDeviceDescriptorIndexingFeaturesEXT descriptorIndexingFeatures {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT,
        .pNext = &extendedDynamicStateFeatures,
        .shaderSampledImageArrayNonUniformIndexing = VK_FALSE,
        .descriptorBindingPartiallyBound = VK_TRUE,
        // Used for specifying the actual size of a shader unsized descriptor array during runtime.
        // The unsized ds array must be the last descriptor in the set
        .descriptorBindingVariableDescriptorCount = VK_FALSE,
        // allows declaring ds arrays in the shader without a fixed size
        .runtimeDescriptorArray = VK_FALSE
    };

    float queuePriority = 1.f;
    VkDeviceQueueCreateInfo queueCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = mGraphicsQueueFamilyIndex,
        .queueCount = 1,
        .pQueuePriorities = &queuePriority
    };

    VkPhysicalDeviceFeatures features {
        .geometryShader = VK_TRUE,
        .samplerAnisotropy = VK_TRUE,
        .fragmentStoresAndAtomics = VK_TRUE,
    };

    VkDeviceCreateInfo deviceCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = &descriptorIndexingFeatures,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &queueCreateInfo,
        .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
        .ppEnabledExtensionNames = extensions.data(),
        .pEnabledFeatures = &features
    };

    VkResult result = vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device);
    vulkanCheck(result, "Failed to create logical device.");

    vkGetDeviceQueue(device, mGraphicsQueueFamilyIndex, 0, &graphicsQueue);
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
            mGraphicsQueueFamilyIndex = i;
            break;
        }
    }
}

void VulkanRenderDevice::createCommandPool()
{
    VkCommandPoolCreateInfo commandPoolCreateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = mGraphicsQueueFamilyIndex
    };

    VkResult result = vkCreateCommandPool(device, &commandPoolCreateInfo, nullptr, &commandPool);
    vulkanCheck(result, "Failed to create command pool.");
}

void VulkanRenderDevice::createDescriptorPool()
{
    uint32_t maxSets = 500;

    std::vector<VkDescriptorPoolSize> poolSizes {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
        {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 100},
        {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 100},
        {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 100},
    };

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = maxSets,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data()
    };

    VkResult result = vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &descriptorPool);
    vulkanCheck(result, "Failed to create descriptor pool.");

    setVulkanObjectDebugName(*this,
                             VK_OBJECT_TYPE_DESCRIPTOR_POOL,
                             "VulkanRenderDevice::descriptorPool",
                             descriptorPool);
}

const VkPhysicalDeviceProperties &VulkanRenderDevice::getDeviceProperties() const
{
    return mDeviceProperties;
}

const VkPhysicalDeviceMemoryProperties &VulkanRenderDevice::getMemoryProperties() const
{
    return mMemoryProperties;
}

uint32_t VulkanRenderDevice::getGraphicsQueueFamilyIndex() const
{
    return mGraphicsQueueFamilyIndex;
}
