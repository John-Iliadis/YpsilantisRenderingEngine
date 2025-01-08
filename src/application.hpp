//
// Created by Gianni on 2/01/2025.
//

#ifndef VULKANRENDERINGENGINE_APPLICATION_HPP
#define VULKANRENDERINGENGINE_APPLICATION_HPP

#include "window/window.hpp"
#include "renderer/renderer.hpp"
#include "vk/include.hpp"
#include "utils.hpp"

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
    void fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void render();

    void countFPS(float dt);

private:
    Window mWindow;
    Renderer mRenderer;

    VulkanInstance mInstance;
    VulkanRenderDevice mRenderDevice;
    VulkanSwapchain mSwapchain;
};

#endif //VULKANRENDERINGENGINE_APPLICATION_HPP
