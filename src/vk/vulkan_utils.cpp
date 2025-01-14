//
// Created by Gianni on 3/01/2025.
//

#include "vulkan_utils.hpp"

void vulkanCheck(VkResult result, const char* msg, std::source_location location)
{
    check(result == VK_SUCCESS, msg, location);
}

void setDebugVulkanObjectName(VkDevice device, VkObjectType type, const std::string& name, const void* handle)
{
#ifdef DEBUG_MODE
    VkDebugUtilsObjectNameInfoEXT objectNameInfo {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = type,
        .objectHandle = (uint64_t)handle,
        .pObjectName = name.c_str()
    };

    VkResult result = pfnSetDebugUtilsObjectNameEXT(device, &objectNameInfo);
    vulkanCheck(result, "Failed to create Vulkan object name.");
#endif
}

std::optional<uint32_t> findSuitableMemoryType(VkPhysicalDeviceMemoryProperties memoryProperties,
                                               uint32_t resourceSupportedMemoryTypes,
                                               VkMemoryPropertyFlags desiredMemoryProperties)
{
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
    {
        if (resourceSupportedMemoryTypes & (1 << i) &&
            (desiredMemoryProperties & memoryProperties.memoryTypes[i].propertyFlags) == desiredMemoryProperties)
            return i;
    }

    return std::optional<uint32_t>();
}

VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool)
{
    VkCommandBuffer commandBuffer;

    VkCommandBufferAllocateInfo commandBufferAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer);

    VkCommandBufferBeginInfo commandBufferBeginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

    return commandBuffer;
}

void endSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkQueue queue)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
    };

    VkResult result = vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vulkanCheck(result, "Failed queue submit.");
    vkQueueWaitIdle(queue);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

VkSemaphore createSemaphore(VkDevice device, const char* tag)
{
    VkSemaphore semaphore;

    VkSemaphoreCreateInfo semaphoreCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0
    };

    VkResult result = vkCreateSemaphore(device, &semaphoreCreateInfo, nullptr, &semaphore);
    vulkanCheck(result, "Failed to create semaphore.");

    if (tag)
    {
        setDebugVulkanObjectName(device, VK_OBJECT_TYPE_SEMAPHORE, tag, semaphore);
    }

    return semaphore;
}

VkFence createFence(VkDevice device, bool signaled, const char* tag)
{
    VkFence fence;

    VkFenceCreateInfo fenceCreateInfo {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = static_cast<VkFenceCreateFlags>(signaled? VK_FENCE_CREATE_SIGNALED_BIT : 0)
    };

    VkResult result = vkCreateFence(device, &fenceCreateInfo, nullptr, &fence);
    vulkanCheck(result, "Failed to create fence.");

    if (tag)
    {
        setDebugVulkanObjectName(device, VK_OBJECT_TYPE_FENCE, tag, fence);
    }

    return fence;
}

VkSampleCountFlagBits getMaxSampleCount(const VkPhysicalDeviceProperties& physicalDeviceProperties)
{
    VkSampleCountFlags sampleCounts = physicalDeviceProperties.limits.framebufferColorSampleCounts &
                                      physicalDeviceProperties.limits.framebufferDepthSampleCounts;

    static VkSampleCountFlagBits flags[] {
        VK_SAMPLE_COUNT_64_BIT,
        VK_SAMPLE_COUNT_32_BIT,
        VK_SAMPLE_COUNT_16_BIT,
        VK_SAMPLE_COUNT_8_BIT,
        VK_SAMPLE_COUNT_4_BIT,
        VK_SAMPLE_COUNT_2_BIT
    };

    for (auto flag : flags)
        if (flag & sampleCounts)
            return flag;

    return VK_SAMPLE_COUNT_1_BIT;
}
