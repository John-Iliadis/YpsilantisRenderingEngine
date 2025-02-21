//
// Created by Gianni on 2/01/2025.
//

#ifndef VULKANRENDERINGENGINE_APPLICATION_HPP
#define VULKANRENDERINGENGINE_APPLICATION_HPP

#include "../window/window.hpp"
#include "../renderer/renderer.hpp"
#include "../renderer/model_importer.hpp"
#include "../vk/vulkan_instance.hpp"
#include "../vk/vulkan_render_device.hpp"
#include "../vk/vulkan_swapchain.hpp"
#include "../vk/vulkan_imgui.hpp"
#include "../utils/utils.hpp"
#include "save_data.hpp"
#include "editor.hpp"

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
    SaveData mSaveData;
    Window mWindow;
    VulkanInstance mInstance;
    VulkanRenderDevice mRenderDevice;
    VulkanSwapchain mSwapchain;
    VulkanImGui mVulkanImGui;
    Renderer mRenderer;
    Editor mEditor;
    ModelImporter mModelImporter;
};

#endif //VULKANRENDERINGENGINE_APPLICATION_HPP
