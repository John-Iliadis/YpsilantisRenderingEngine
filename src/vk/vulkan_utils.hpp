//
// Created by Gianni on 3/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_UTILS_HPP
#define VULKANRENDERINGENGINE_VULKAN_UTILS_HPP

class VulkanRenderDevice;

void vulkanCheck(VkResult result, const char* msg, std::source_location location = std::source_location::current());

void setDebugVulkanObjectName(const VulkanRenderDevice& renderDevice,
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

VkSampleCountFlagBits getMaxSampleCount(const VkPhysicalDeviceProperties& physicalDeviceProperties);

#endif //VULKANRENDERINGENGINE_VULKAN_UTILS_HPP
