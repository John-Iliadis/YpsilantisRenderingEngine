//
// Created by Gianni on 3/01/2025.
//

#include "vulkan_utils.hpp"
#include "vulkan_function_pointers.hpp"
#include "vulkan_render_device.hpp"
#include "../utils/utils.hpp"

void vulkanCheck(VkResult result, const char* msg, std::source_location location)
{
    check(result == VK_SUCCESS, msg, location);
}

void setVulkanObjectDebugName(const VulkanRenderDevice& renderDevice,
                              VkObjectType type,
                              const std::string& name,
                              const void* handle)
{
#ifdef DEBUG_MODE
    VkDebugUtilsObjectNameInfoEXT objectNameInfo {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = type,
        .objectHandle = (uint64_t)handle,
        .pObjectName = name.c_str()
    };

    VkResult result = pfnSetDebugUtilsObjectNameEXT(renderDevice.device, &objectNameInfo);
    vulkanCheck(result, "Failed to create Vulkan object name.");
#endif
}

void beginDebugLabel(VkCommandBuffer commandBuffer, const char* name)
{
#ifdef DEBUG_MODE
    VkDebugUtilsLabelEXT debugUtilsLabel {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pLabelName = name,
        .color {0.f, 1.f, 0.f, 1.f}
    };

    pfnCmdBeginDebugUtilsLabelEXT(commandBuffer, &debugUtilsLabel);
#endif
}

void endDebugLabel(VkCommandBuffer commandBuffer)
{
#ifdef DEBUG_MODE
    pfnCmdEndDebugUtilsLabelEXT(commandBuffer);
#endif
}

void insertDebugLabel(VkCommandBuffer commandBuffer, const char* name)
{
#ifdef DEBUG_MODE
    VkDebugUtilsLabelEXT debugUtilsLabel {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pLabelName = name,
        .color {0.f, 1.f, 0.f, 1.f}
    };

    pfnCmdInsertDebugUtilsLabelEXT(commandBuffer, &debugUtilsLabel);
#endif
}

std::optional<uint32_t> findSuitableMemoryType(const VkPhysicalDeviceMemoryProperties& deviceMemoryProperties,
                                               uint32_t resourceSupportedMemoryTypes,
                                               VkMemoryPropertyFlags desiredMemoryProperties)
{
    for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; ++i)
    {
        if (resourceSupportedMemoryTypes & (1 << i) &&
            (desiredMemoryProperties & deviceMemoryProperties.memoryTypes[i].propertyFlags) == desiredMemoryProperties)
            return i;
    }

    return std::nullopt;
}

VkCommandBuffer beginSingleTimeCommands(const VulkanRenderDevice& renderDevice)
{
    VkCommandBuffer commandBuffer;

    VkCommandBufferAllocateInfo commandBufferAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = renderDevice.commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    vkAllocateCommandBuffers(renderDevice.device, &commandBufferAllocateInfo, &commandBuffer);

    VkCommandBufferBeginInfo commandBufferBeginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

    return commandBuffer;
}

void endSingleTimeCommands(const VulkanRenderDevice& renderDevice, VkCommandBuffer commandBuffer)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
    };

    VkResult result = vkQueueSubmit(renderDevice.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vulkanCheck(result, "Failed queue submit.");
    vkQueueWaitIdle(renderDevice.graphicsQueue);

    vkFreeCommandBuffers(renderDevice.device, renderDevice.commandPool, 1, &commandBuffer);
}

VkSemaphore createSemaphore(const VulkanRenderDevice& renderDevice, const char* tag)
{
    VkSemaphore semaphore;

    VkSemaphoreCreateInfo semaphoreCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0
    };

    VkResult result = vkCreateSemaphore(renderDevice.device, &semaphoreCreateInfo, nullptr, &semaphore);
    vulkanCheck(result, "Failed to create semaphore.");

    if (tag)
    {
        setVulkanObjectDebugName(renderDevice, VK_OBJECT_TYPE_SEMAPHORE, tag, semaphore);
    }

    return semaphore;
}

VkFence createFence(const VulkanRenderDevice& renderDevice, bool signaled, const char* tag)
{
    VkFence fence;

    VkFenceCreateInfo fenceCreateInfo {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = static_cast<VkFenceCreateFlags>(signaled? VK_FENCE_CREATE_SIGNALED_BIT : 0)
    };

    VkResult result = vkCreateFence(renderDevice.device, &fenceCreateInfo, nullptr, &fence);
    vulkanCheck(result, "Failed to create fence.");

    if (tag)
    {
        setVulkanObjectDebugName(renderDevice, VK_OBJECT_TYPE_FENCE, tag, fence);
    }

    return fence;
}

VkSampleCountFlagBits getMaxSampleCount(const VulkanRenderDevice& renderDevice)
{
    const auto& deviceProperties = renderDevice.getDeviceProperties();

    VkSampleCountFlags sampleCounts = deviceProperties.limits.framebufferColorSampleCounts &
                                      deviceProperties.limits.framebufferDepthSampleCounts;

    static const VkSampleCountFlagBits flags[] {
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

void setRenderpassDebugName(const VulkanRenderDevice& renderDevice, VkRenderPass renderPass, const std::string& name)
{
    setVulkanObjectDebugName(renderDevice, VK_OBJECT_TYPE_RENDER_PASS, name, renderPass);
}

void setFramebufferDebugName(const VulkanRenderDevice& renderDevice, VkFramebuffer framebuffer, const std::string& name)
{
    setVulkanObjectDebugName(renderDevice, VK_OBJECT_TYPE_FRAMEBUFFER, name, framebuffer);
}