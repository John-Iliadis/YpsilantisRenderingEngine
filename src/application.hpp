//
// Created by Gianni on 2/01/2025.
//

#ifndef VULKANRENDERINGENGINE_APPLICATION_HPP
#define VULKANRENDERINGENGINE_APPLICATION_HPP

#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "camera/camera.hpp"
#include "vk/include.hpp"
#include "renderer/cube_renderer.hpp"
#include "input.hpp"
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

    void update(float dt);
    void fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void render();

    void countFPS(float dt);

    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double x, double y);
    static void mouseScrollCallback(GLFWwindow* window, double x, double y);

private:
    GLFWwindow* mWindow;
    VulkanInstance mInstance;
    VulkanRenderDevice mRenderDevice;
    VulkanSwapchain mSwapchain;
    VkDescriptorPool mDescriptorPool;

    std::vector<VulkanImage> mDepthImages;

    VkDescriptorSetLayout mViewProjectionDSLayout;
    std::vector<VkDescriptorSet> mViewProjectionDS;
    std::vector<VulkanBuffer> mViewProjectionUBOs;

    uint32_t mCurrentFrame;
    CubeRenderer mCubeRenderer;
    Camera mSceneCamera;
};

#endif //VULKANRENDERINGENGINE_APPLICATION_HPP
