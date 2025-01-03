//
// Created by Gianni on 2/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_RENDER_DEVICE_HPP
#define VULKANRENDERINGENGINE_VULKAN_RENDER_DEVICE_HPP

#include <vulkan/vulkan.h>
#include "vulkan_instance.hpp"
#include "vulkan_utils.hpp"

class VulkanRenderDevice
{
public:
    VkPhysicalDevice physicalDevice;
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    VkPhysicalDeviceMemoryProperties memoryProperties;

    VkDevice device;
    VkQueue graphicsQueue;

    uint32_t graphicsQueueFamilyIndex;

public:
    void create(const VulkanInstance& instance);
    void destroy();

private:
    void pickPhysicalDevice(const VulkanInstance& instance);
    void createLogicalDevice();
    void findQueueFamilyIndices();
};

#endif //VULKANRENDERINGENGINE_VULKAN_RENDER_DEVICE_HPP
