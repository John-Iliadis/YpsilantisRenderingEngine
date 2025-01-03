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

    return VulkanBuffer(buffer, bufferMemory, size);
}

VulkanBuffer createBufferWithStaging(const VulkanRenderDevice& renderDevice,
                                     VkDeviceSize size,
                                     VkBufferUsageFlags usage,
                                     VkMemoryPropertyFlags memoryProperties,
                                     void* bufferData)
{
    VkBufferUsageFlags stagingBufferUsage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

    VkMemoryPropertyFlags stagingBufferMemoryProperties {
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    };

    VulkanBuffer stagingBuffer = createBuffer(renderDevice,
                                              size,
                                              stagingBufferUsage,
                                              stagingBufferMemoryProperties);\
    mapBufferMemory(renderDevice, stagingBuffer, 0, size, bufferData);

    VulkanBuffer buffer = createBuffer(renderDevice,
                                       size,
                                       usage,
                                       memoryProperties);

    copyBuffer(renderDevice, stagingBuffer, buffer, size);

    destroyBuffer(renderDevice, stagingBuffer);

    return buffer;
}

void destroyBuffer(const VulkanRenderDevice& renderDevice, VulkanBuffer& buffer)
{
    vkDestroyBuffer(renderDevice.device, buffer.buffer, nullptr);
    vkFreeMemory(renderDevice.device, buffer.memory, nullptr);
    buffer.size = 0;
}

void mapBufferMemory(const VulkanRenderDevice& renderDevice,
                     VulkanBuffer& buffer,
                     VkDeviceSize offset,
                     VkDeviceSize size,
                     void* data)
{
    void* dataPtr;
    vkMapMemory(renderDevice.device, buffer.memory, offset, size, 0, &dataPtr);
    memcpy(dataPtr, data, size);
    vkUnmapMemory(renderDevice.device, buffer.memory);
}

void copyBuffer(const VulkanRenderDevice& renderDevice,
                VulkanBuffer& srcBuffer,
                VulkanBuffer& dstBuffer,
                VkDeviceSize size)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(renderDevice.device, renderDevice.commandPool);

    VkBufferCopy copyRegion {
        .srcOffset {},
        .dstOffset {},
        .size = size
    };

    vkCmdCopyBuffer(commandBuffer, srcBuffer.buffer, dstBuffer.buffer, 1, &copyRegion);

    endSingleTimeCommands(renderDevice.device, renderDevice.commandPool, commandBuffer, renderDevice.graphicsQueue);
}
