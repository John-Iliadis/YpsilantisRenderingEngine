//
// Created by Gianni on 7/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_FUNCTION_POINTERS_HPP
#define VULKANRENDERINGENGINE_VULKAN_FUNCTION_POINTERS_HPP

// VK_EXT_debug_utils
inline PFN_vkCreateDebugUtilsMessengerEXT pfnCreateDebugUtilsMessengerEXT;
inline PFN_vkDestroyDebugUtilsMessengerEXT pfnDestroyDebugUtilsMessengerEXT;
inline PFN_vkSetDebugUtilsObjectNameEXT pfnSetDebugUtilsObjectNameEXT;
inline PFN_vkCmdBeginDebugUtilsLabelEXT pfnCmdBeginDebugUtilsLabelEXT;
inline PFN_vkCmdEndDebugUtilsLabelEXT pfnCmdEndDebugUtilsLabelEXT;
inline PFN_vkCmdInsertDebugUtilsLabelEXT pfnCmdInsertDebugUtilsLabelEXT;

// VK_EXT_extended_dynamic_state
inline PFN_vkCmdSetCullModeEXT pfnCmdSetCullModeEXT;
inline PFN_vkCmdSetFrontFaceEXT pfnCmdSetFrontFaceEXT;

// VK_EXT_host_query_reset
inline PFN_vkResetQueryPoolEXT pfnResetQueryPoolEXT;

// VK_KHR_push_descriptor
inline PFN_vkCmdPushDescriptorSet pfnCmdPushDescriptorSet;

inline void loadExtensionFunctionsPointers(VkInstance instance)
{
#ifdef DEBUG_MODE
    pfnCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    pfnDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    pfnSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT"));
    pfnCmdBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT"));
    pfnCmdEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT"));
    pfnCmdInsertDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT"));
#endif

    pfnCmdSetCullModeEXT = reinterpret_cast<PFN_vkCmdSetCullModeEXT>(vkGetInstanceProcAddr(instance, "vkCmdSetCullModeEXT"));
    pfnCmdSetFrontFaceEXT = reinterpret_cast<PFN_vkCmdSetFrontFaceEXT>(vkGetInstanceProcAddr(instance, "vkCmdSetFrontFaceEXT"));

    pfnResetQueryPoolEXT = reinterpret_cast<PFN_vkResetQueryPoolEXT>(vkGetInstanceProcAddr(instance, "vkResetQueryPoolEXT"));

    pfnCmdPushDescriptorSet = reinterpret_cast<PFN_vkCmdPushDescriptorSet>(vkGetInstanceProcAddr(instance, "vkCmdPushDescriptorSetKHR"));
}

#endif //VULKANRENDERINGENGINE_VULKAN_FUNCTION_POINTERS_HPP
