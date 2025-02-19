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
#include "../window/window.hpp"
#include "vulkan_instance.hpp"
#include "vulkan_render_device.hpp"
#include "vulkan_swapchain.hpp"

class VulkanImGui
{
public:
    VulkanImGui(const Window& window,
                const VulkanInstance& instance,
                const VulkanRenderDevice& renderDevice,
                const VulkanSwapchain& swapchain);
    ~VulkanImGui();

    void begin();
    void end();
    void onSwapchainRecreate();
    void render(VkCommandBuffer commandBuffer, uint32_t imageIndex);

private:
    void createRenderpass();
    void createFramebuffers();

private:
    const VulkanRenderDevice& mRenderDevice;
    const VulkanSwapchain& mSwapchain;

    VkRenderPass mRenderpass;
    std::array<VkFramebuffer, 2> mFramebuffers;
};

#endif //VULKANRENDERINGENGINE_VULKAN_IMGUI_HPP
