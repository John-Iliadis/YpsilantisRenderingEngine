//
// Created by Gianni on 3/01/2025.
//

#include "vulkan_utils.hpp"

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

std::optional<uint32_t> findSuitableMemoryType(VkPhysicalDeviceMemoryProperties memoryProperties,
                                               uint32_t resourceSupportedMemoryTypes,
                                               VkMemoryPropertyFlags desiredMemoryProperties)
{
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i)
    {
        if (resourceSupportedMemoryTypes & (1 << i) &&
            (desiredMemoryProperties & memoryProperties.memoryTypes[i].propertyFlags) == desiredMemoryProperties)
            return i;
    }

    return std::optional<uint32_t>();
}

VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool)
{
    VkCommandBuffer commandBuffer;

    VkCommandBufferAllocateInfo commandBufferAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer);

    VkCommandBufferBeginInfo commandBufferBeginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);

    return commandBuffer;
}

void endSingleTimeCommands(VkDevice device, VkCommandPool commandPool, VkCommandBuffer commandBuffer, VkQueue queue)
{
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer,
    };

    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkDeviceWaitIdle(device);

    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

VkShaderModule createShaderModule(VkDevice device, const std::string& filename)
{
    std::ifstream spirv(filename, std::ios::binary | std::ios::ate);

    check(spirv.is_open(), std::format("Failed to open {}", filename).c_str());

    size_t byteCount = spirv.tellg();
    char* code = new char[byteCount];
    spirv.seekg(0);
    spirv.read(code, byteCount);

    VkShaderModule shaderModule;

    VkShaderModuleCreateInfo shaderModuleCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = byteCount,
        .pCode = reinterpret_cast<uint32_t*>(code)
    };

    VkResult result = vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &shaderModule);
    vulkanCheck(result, "Failed to create shader module.");

    delete[] code;

    return shaderModule;
}
