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

#endif //VULKANRENDERINGENGINE_VULKAN_UTILS_HPP
