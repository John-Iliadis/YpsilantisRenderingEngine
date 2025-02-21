//
// Created by Gianni on 3/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_UTILS_HPP
#define VULKANRENDERINGENGINE_VULKAN_UTILS_HPP

class VulkanRenderDevice;

void vulkanCheck(VkResult result, const char* msg, std::source_location location = std::source_location::current());

void setVulkanObjectDebugName(const VulkanRenderDevice& renderDevice,
                              VkObjectType type,
                              const std::string& name,
                              const void* handle);

void beginDebugLabel(VkCommandBuffer commandBuffer, const char* name);
void endDebugLabel(VkCommandBuffer commandBuffer);
void insertDebugLabel(VkCommandBuffer commandBuffer, const char* name);

std::optional<uint32_t> findSuitableMemoryType(const VkPhysicalDeviceMemoryProperties& deviceMemoryProperties,
                                               uint32_t resourceSupportedMemoryTypes,
                                               VkMemoryPropertyFlags desiredMemoryProperties);

VkCommandBuffer beginSingleTimeCommands(const VulkanRenderDevice& renderDevice);
void endSingleTimeCommands(const VulkanRenderDevice& renderDevice, VkCommandBuffer commandBuffer);

VkSemaphore createSemaphore(const VulkanRenderDevice& renderDevice, const char* tag = nullptr);
VkFence createFence(const VulkanRenderDevice& renderDevice, bool signaled = false, const char* tag = nullptr);

VkSampleCountFlagBits getMaxSampleCount(const VulkanRenderDevice& renderDevice);

void setRenderpassDebugName(const VulkanRenderDevice& renderDevice, VkRenderPass renderPass, const std::string& name);
void setFramebufferDebugName(const VulkanRenderDevice& renderDevice, VkFramebuffer framebuffer, const std::string& name);

VkDeviceSize formatSize(VkFormat format);

VkDeviceSize imageMemoryDeviceSize(uint32_t width, uint32_t height, VkFormat format);

template<typename T>
std::vector<T> readBufferToVector(VkDevice device, VkDeviceMemory bufferMemory, VkDeviceSize bufferSize)
{
    assert(bufferSize % sizeof(T) != 0);

    void* data;
    VkResult result = vkMapMemory(device, bufferMemory, 0, bufferSize, 0, &data);
    vulkanCheck(result, "Failed to map Vulkan buffer memory.");

    size_t elementCount = bufferSize / sizeof(T);
    std::vector<T> output(elementCount);
    memcpy(output.data(), data, static_cast<size_t>(bufferSize));

    vkUnmapMemory(device, bufferMemory);

    return output;
}

#endif //VULKANRENDERINGENGINE_VULKAN_UTILS_HPP
