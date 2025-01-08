//
// Created by Gianni on 2/01/2025.
//

#ifndef VULKANRENDERINGENGINE_APPLICATION_HPP
#define VULKANRENDERINGENGINE_APPLICATION_HPP

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "window/window.hpp"
#include "camera/camera.hpp"
#include "vk/include.hpp"
#include "renderer/cube_renderer.hpp"
#include "renderer/render_finish.hpp"
#include "window/input.hpp"
#include "utils.hpp"

class Application
{
public:
    Application();
    ~Application();
    void run();

private:
    void initializeGLFW();
    void initializeVulkan();
    void createDepthImages();
    void createDescriptorPool();
    void createViewProjUBOs();
    void createViewProjDescriptors();
    void updateUniformBuffers();

    void recreateSwapchain();

    void handleEvents();
    void update(float dt);
    void fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void render();

    void countFPS(float dt);

private:
    Window mWindow;
    VulkanInstance mInstance;
    VulkanRenderDevice mRenderDevice;
    VulkanSwapchain mSwapchain;
    VkDescriptorPool mDescriptorPool;

    std::vector<VulkanImage> mDepthImages;

    VkDescriptorSetLayout mViewProjectionDSLayout;
    std::vector<VkDescriptorSet> mViewProjectionDS;
    std::vector<VulkanBuffer> mViewProjectionUBOs;

    uint32_t mCurrentFrame;
    Camera mSceneCamera;

    CubeRenderer mCubeRenderer;
    RenderFinish mRenderFinish;
};

#endif //VULKANRENDERINGENGINE_APPLICATION_HPP
