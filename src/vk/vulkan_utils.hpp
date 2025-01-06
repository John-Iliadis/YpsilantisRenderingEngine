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

std::optional<uint32_t> findSuitableMemoryType(VkPhysicalDeviceMemoryProperties memoryProperties,
                                               uint32_t resourceSupportedMemoryTypes,
                                               VkMemoryPropertyFlags desiredMemoryProperties);

VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);
void endSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkQueue queue);

#endif //VULKANRENDERINGENGINE_VULKAN_UTILS_HPP
