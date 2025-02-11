//
// Created by Gianni on 2/01/2025.
//

#ifndef VULKANRENDERINGENGINE_APPLICATION_HPP
#define VULKANRENDERINGENGINE_APPLICATION_HPP

#include "../window/window.hpp"
#include "../renderer/renderer.hpp"
#include "editor.hpp"
#include "../vk/vulkan_instance.hpp"
#include "../vk/vulkan_render_device.hpp"
#include "../vk/vulkan_swapchain.hpp"
#include "../vk/vulkan_imgui.hpp"
#include "../utils/utils.hpp"

class Application
{
public:
    Application();
    ~Application();
    void run();

private:
    void recreateSwapchain();
    void handleEvents();
    void update(float dt);
    void fillCommandBuffer(uint32_t imageIndex);
    void render();

private:
    Window mWindow;
    VulkanInstance mInstance;
    VulkanRenderDevice mRenderDevice;
    VulkanSwapchain mSwapchain;
    VulkanImGui mVulkanImGui;
    Renderer mRenderer;
    Editor mEditor;
};

#endif //VULKANRENDERINGENGINE_APPLICATION_HPP
