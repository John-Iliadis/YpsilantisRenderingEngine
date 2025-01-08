//
// Created by Gianni on 4/01/2025.
//

#ifndef VULKANRENDERINGENGINE_RENDERER_HPP
#define VULKANRENDERINGENGINE_RENDERER_HPP

#include <vulkan/vulkan.h>
#include <glm/gtc/type_ptr.hpp>
#include "../vk/include.hpp"
#include "../camera/camera.hpp"

class Renderer
{
public:
    void init(GLFWwindow* window,
              const VulkanInstance& instance,
              const VulkanRenderDevice& renderDevice,
              const VulkanSwapchain& swapchain);
    void terminate();

    void handleEvent(const Event& event);
    void update(float dt);
    void fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);
    void onSwapchainRecreate();

private:
    void createDescriptorPool();
    void createDepthImages();
    void createViewProjUBOs();
    void createViewProjDescriptors();

    void createImguiFramebuffers();
    void createImguiRenderpass();

    void renderImgui(VkCommandBuffer commandBuffer, uint32_t imageIndex);

    void updateUniformBuffers(uint32_t imageIndex);

private:
    const VulkanRenderDevice* mRenderDevice;
    const VulkanSwapchain* mSwapchain;
    VkDescriptorPool mDescriptorPool;

    VkRenderPass mImguiRenderpass;

    VkDescriptorSetLayout mViewProjDSLayout;
    std::vector<VkDescriptorSet> mViewProjDS;
    std::vector<VulkanBuffer> mViewProjUBOs;

    Camera mSceneCamera;

    std::vector<VkFramebuffer> mImguiFramebuffers;
    std::vector<VulkanImage> mDepthImages;
};

#endif //VULKANRENDERINGENGINE_RENDERER_HPP
