//
// Created by Gianni on 2/01/2025.
//

#include "utils.hpp"



void debugLog(const std::string& logMSG)
{
    std::cout << "[Debug Log] : "<< logMSG << std::endl;
}

void check(bool result, const char* msg, std::source_location location)
{
    if (!result)
    {
        std::stringstream errorMSG;
        errorMSG << '`' << location.function_name() << "`: " << msg << '\n';

        throw std::runtime_error(errorMSG.str());
    }
}

void vulkanCheck(VkResult result, const char* msg, std::source_location location)
{
    check(result == VK_SUCCESS, msg, location);
}

void loadDebugUtilsFunctionPointers(VkInstance instance)
{
#ifdef DEBUG_MODE
    pfnSetDebugUtilsObjectNameEXT = reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT"));
    pfnCmdBeginDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdBeginDebugUtilsLabelEXT"));
    pfnCmdEndDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdEndDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdEndDebugUtilsLabelEXT"));
    pfnCmdInsertDebugUtilsLabelEXT = reinterpret_cast<PFN_vkCmdInsertDebugUtilsLabelEXT>(vkGetInstanceProcAddr(instance, "vkCmdInsertDebugUtilsLabelEXT"));
#endif
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
