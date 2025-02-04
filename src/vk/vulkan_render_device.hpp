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
    VkDevice device;
    VkQueue graphicsQueue;
    VkCommandPool commandPool;
    VkDescriptorPool descriptorPool;

public:
    VulkanRenderDevice(const VulkanInstance& instance);
    ~VulkanRenderDevice();

    const VkPhysicalDeviceProperties& getDeviceProperties() const;
    const VkPhysicalDeviceMemoryProperties& getMemoryProperties() const;
    uint32_t getGraphicsQueueFamilyIndex() const;

private:
    void pickPhysicalDevice(const VulkanInstance& instance);
    void createLogicalDevice();
    void findQueueFamilyIndices();
    void createCommandPool();
    void createDescriptorPool();

private:
    VkPhysicalDeviceProperties mDeviceProperties;
    VkPhysicalDeviceMemoryProperties mMemoryProperties;
    uint32_t mGraphicsQueueFamilyIndex;
};

#endif //VULKANRENDERINGENGINE_VULKAN_RENDER_DEVICE_HPP
