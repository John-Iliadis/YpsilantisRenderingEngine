//
// Created by Gianni on 2/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_DEVICE_HPP
#define VULKANRENDERINGENGINE_VULKAN_DEVICE_HPP

#include <vulkan/vulkan.h>
#include "vulkan_instance.hpp"
#include "../utils.hpp"

class VulkanPhysicalDevice
{
public:
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memoryProperties;
    uint32_t graphicsQueueFamilyIndex;

public:
    void create(const VulkanInstance& instance);

private:
    void pickPhysicalDevice(const VulkanInstance& instance);
    void findQueueFamilyIndices();
};


class VulkanDevice
{
public:
    VkDevice device;
    VkQueue graphicsQueue;

public:
    void create(const VulkanPhysicalDevice& physicalDevice);
    void destroy();
};

#endif //VULKANRENDERINGENGINE_VULKAN_DEVICE_HPP
