//
// Created by Gianni on 3/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_UTILS_HPP
#define VULKANRENDERINGENGINE_VULKAN_UTILS_HPP

#include <vulkan/vulkan.h>
#include "vulkan_function_pointers.hpp"
#include "../utils.hpp"

void vulkanCheck(VkResult result, const char* msg, std::source_location location = std::source_location::current());

void setDebugVulkanObjectName(VkDevice device, VkObjectType type, const std::string& name, const void* handle);
void beginDebugLabel(VkCommandBuffer commandBuffer, const char* name);
void endDebugLabel(VkCommandBuffer commandBuffer);
void insertDebugLabel(VkCommandBuffer commandBuffer, const char* name);

std::optional<uint32_t> findSuitableMemoryType(VkPhysicalDeviceMemoryProperties memoryProperties,
                                               uint32_t resourceSupportedMemoryTypes,
                                               VkMemoryPropertyFlags desiredMemoryProperties);

VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);
void endSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkQueue queue);

VkDescriptorPool createDescriptorPool(VkDevice device,
                                      uint32_t imageSamplerCount,
                                      uint32_t uniformBufferCount,
                                      uint32_t storageBufferCount,
                                      uint32_t maxSets);

VkDescriptorPoolSize descriptorPoolSize(VkDescriptorType type, uint32_t count);

VkSemaphore createSemaphore(VkDevice device, const char* tag = nullptr);
VkFence createFence(VkDevice device, bool signaled = false, const char* tag = nullptr);

VkSampleCountFlagBits getMaxSampleCount(const VkPhysicalDeviceProperties& physicalDeviceProperties);

#endif //VULKANRENDERINGENGINE_VULKAN_UTILS_HPP
