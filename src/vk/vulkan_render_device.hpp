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
    VkPhysicalDeviceMemoryProperties memoryProperties;

    VkDevice device;
    VkQueue graphicsQueue;
    VkCommandPool commandPool;

    uint32_t graphicsQueueFamilyIndex;

public:
    void create(const VulkanInstance& instance);
    void destroy();

    static bool checkExtSupport(VkPhysicalDevice physicalDevice, const char* extension);
    static bool checkExtSupport(VkPhysicalDevice physicalDevice, const std::vector<const char*>& extensions);
    static std::vector<const char*> getRequiredExtensions();

private:
    void pickPhysicalDevice(const VulkanInstance& instance);
    void createLogicalDevice();
    void findQueueFamilyIndices();
    void createCommandPool();
};

#endif //VULKANRENDERINGENGINE_VULKAN_RENDER_DEVICE_HPP
