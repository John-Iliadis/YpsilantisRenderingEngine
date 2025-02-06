//
// Created by Gianni on 2/01/2025.
//

#ifndef VULKANRENDERINGENGINE_APPLICATION_HPP
#define VULKANRENDERINGENGINE_APPLICATION_HPP

#include "../window/window.hpp"
#include "../renderer/renderer.hpp"
#include "../resource/resource_manager.hpp"
#include "../editor/editor.hpp"
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
    void fillCommandBuffer(VkCommandBuffer commandBuffer);
    void render();

    void countFPS(float dt);

    void createRenderPass();
    void createSwapchainFramebuffers();

private:
    Window mWindow;
    VulkanInstance mInstance;
    std::shared_ptr<VulkanRenderDevice> mRenderDevice;
    VulkanSwapchain mSwapchain;

    std::shared_ptr<Renderer> mRenderer;
    std::shared_ptr<ResourceManager> mResourceManager;

    Editor mEditor;
    VulkanImGui mVulkanImGui;

    VkRenderPass mRenderPass;
    std::array<VkFramebuffer, 2> mSwapchainFramebuffers;
};

#endif //VULKANRENDERINGENGINE_APPLICATION_HPP
