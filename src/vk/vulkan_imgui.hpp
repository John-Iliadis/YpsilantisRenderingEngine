//
// Created by Gianni on 8/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_IMGUI_HPP
#define VULKANRENDERINGENGINE_VULKAN_IMGUI_HPP

#include <glfw/glfw3.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_vulkan.h>
#include "vulkan_instance.hpp"
#include "vulkan_render_device.hpp"
#include "vulkan_swapchain.hpp"

void initImGui(GLFWwindow* window,
               const VulkanInstance& instance,
               const VulkanRenderDevice& renderDevice,
               VkDescriptorPool descriptorPool,
               VkRenderPass renderPass);
void terminateImGui();
void beginImGui();
void endImGui();
void imGuiFillCommandBuffer(VkCommandBuffer commandBuffer);

#endif //VULKANRENDERINGENGINE_VULKAN_IMGUI_HPP
