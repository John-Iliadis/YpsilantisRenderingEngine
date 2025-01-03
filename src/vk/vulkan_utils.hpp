//
// Created by Gianni on 3/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_UTILS_HPP
#define VULKANRENDERINGENGINE_VULKAN_UTILS_HPP

#include <vulkan/vulkan.h>
#include "../utils.hpp"

inline PFN_vkSetDebugUtilsObjectNameEXT pfnSetDebugUtilsObjectNameEXT;
inline PFN_vkCmdBeginDebugUtilsLabelEXT pfnCmdBeginDebugUtilsLabelEXT;
inline PFN_vkCmdEndDebugUtilsLabelEXT pfnCmdEndDebugUtilsLabelEXT;
inline PFN_vkCmdInsertDebugUtilsLabelEXT pfnCmdInsertDebugUtilsLabelEXT;

void vulkanCheck(VkResult result, const char* msg, std::source_location location = std::source_location::current());

void loadDebugUtilsFunctionPointers(VkInstance instance);
void setDebugVulkanObjectName(VkDevice device, VkObjectType type, const std::string& name, const void* handle);

std::optional<uint32_t> findSuitableMemoryType(VkPhysicalDeviceMemoryProperties memoryProperties,
                                               uint32_t resourceSupportedMemoryTypes,
                                               VkMemoryPropertyFlags desiredMemoryProperties);

VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);
void endSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkQueue queue);

VkShaderModule createShaderModule(VkDevice device, const std::string& filename);

#endif //VULKANRENDERINGENGINE_VULKAN_UTILS_HPP
