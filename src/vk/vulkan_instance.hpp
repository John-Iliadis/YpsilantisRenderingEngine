//
// Created by Gianni on 2/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_INSTANCE_HPP
#define VULKANRENDERINGENGINE_VULKAN_INSTANCE_HPP

#include <glfw/glfw3.h>
#include <vulkan/vulkan.h>
#include "vulkan_function_pointers.hpp"
#include "vulkan_utils.hpp"

class VulkanInstance
{
public:
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;

public:
    VulkanInstance();
    ~VulkanInstance();

    static VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity,
                                             VkDebugUtilsMessageTypeFlagsEXT type,
                                             const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
                                             void *userPointer);
};

#endif //VULKANRENDERINGENGINE_VULKAN_INSTANCE_HPP
