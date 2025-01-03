//
// Created by Gianni on 3/01/2025.
//

#include "vulkan_buffer.hpp"

VulkanBuffer createBuffer(const VulkanRenderDevice& renderDevice,
                          VkDeviceSize size,
                          VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags memoryProperties)
{
    VkBuffer buffer;
    VkDeviceMemory bufferMemory;

    VkBufferCreateInfo bufferCreateInfo {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE
    };

    VkResult result = vkCreateBuffer(renderDevice.device, &bufferCreateInfo, nullptr, &buffer);
    vulkanCheck(result, "Failed to create staging buffer.");

    VkMemoryRequirements stagingBufferMemoryRequirements;
    vkGetBufferMemoryRequirements(renderDevice.device, buffer, &stagingBufferMemoryRequirements);

    uint32_t stagingBufferMemoryTypeIndex = findSuitableMemoryType(renderDevice.memoryProperties,
                                                                   stagingBufferMemoryRequirements.memoryTypeBits,
                                                                   memoryProperties).value();

    VkMemoryAllocateInfo memoryAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = stagingBufferMemoryRequirements.size,
        .memoryTypeIndex = stagingBufferMemoryTypeIndex
    };

    result = vkAllocateMemory(renderDevice.device, &memoryAllocateInfo, nullptr, &bufferMemory);
    vulkanCheck(result, "Failed to allocate staging buffer memory.");

    vkBindBufferMemory(renderDevice.device, buffer, bufferMemory, 0);

    return VulkanBuffer(buffer, bufferMemory);
}
