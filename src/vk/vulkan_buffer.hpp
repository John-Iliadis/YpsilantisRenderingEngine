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

VulkanBuffer createBufferWithStaging(const VulkanRenderDevice& renderDevice,
                                     VkDeviceSize size,
                                     VkBufferUsageFlags usage,
                                     VkMemoryPropertyFlags memoryProperties,
                                     const void* bufferData);

void destroyBuffer(const VulkanRenderDevice& renderDevice, VulkanBuffer& buffer);

void mapBufferMemory(const VulkanRenderDevice& renderDevice,
                     VulkanBuffer& buffer,
                     VkDeviceSize offset,
                     VkDeviceSize size,
                     const void* data);

void copyBuffer(const VulkanRenderDevice& renderDevice,
                VulkanBuffer& srcBuffer,
                VulkanBuffer& dstBuffer,
                VkDeviceSize size);

#endif //VULKANRENDERINGENGINE_VULKAN_BUFFER_HPP
