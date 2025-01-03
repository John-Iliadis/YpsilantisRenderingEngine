//
// Created by Gianni on 2/01/2025.
//

#ifndef VULKANRENDERINGENGINE_UTILS_HPP
#define VULKANRENDERINGENGINE_UTILS_HPP

#include <vulkan/vulkan.h>

inline PFN_vkSetDebugUtilsObjectNameEXT pfnSetDebugUtilsObjectNameEXT;
inline PFN_vkCmdBeginDebugUtilsLabelEXT pfnCmdBeginDebugUtilsLabelEXT;
inline PFN_vkCmdEndDebugUtilsLabelEXT pfnCmdEndDebugUtilsLabelEXT;
inline PFN_vkCmdInsertDebugUtilsLabelEXT pfnCmdInsertDebugUtilsLabelEXT;

void debugLog(const std::string& logMSG);

void check(bool result, const char* msg, std::source_location location = std::source_location::current());
void vulkanCheck(VkResult result, const char* msg, std::source_location location = std::source_location::current());

void loadDebugUtilsFunctionPointers(VkInstance instance);
void setDebugVulkanObjectName(VkDevice device, VkObjectType type, const std::string& name, const void* handle);

#endif //VULKANRENDERINGENGINE_UTILS_HPP
