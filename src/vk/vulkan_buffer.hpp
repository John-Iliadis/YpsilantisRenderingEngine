//
// Created by Gianni on 3/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_BUFFER_HPP
#define VULKANRENDERINGENGINE_VULKAN_BUFFER_HPP

#include <vulkan/vulkan.h>
#include "vulkan_render_device.hpp"

struct VulkanBuffer
{
    VkBuffer buffer;
    VkDeviceMemory memory;
    VkDeviceSize size;
};

VulkanBuffer createBuffer(const VulkanRenderDevice& renderDevice,
                          VkDeviceSize size,
                          VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags memoryProperties);

#endif //VULKANRENDERINGENGINE_VULKAN_BUFFER_HPP
