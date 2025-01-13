//
// Created by Gianni on 4/01/2025.
//

#ifndef VULKANRENDERINGENGINE_RENDERER_HPP
#define VULKANRENDERINGENGINE_RENDERER_HPP

#include <vulkan/vulkan.h>
#include <glm/gtc/type_ptr.hpp>
#include "../vk/include.hpp"
#include "camera.hpp"
#include "scene.hpp"
#include "scene_node.hpp"

class Renderer
{
public:
    void init(GLFWwindow* window,
              VulkanInstance& instance,
              VulkanRenderDevice& renderDevice,
              VulkanSwapchain& swapchain);
    void terminate();

    void handleEvent(const Event& event);
    void update(float dt);
    void fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t frameIndex, uint32_t swapchainImageIndex);
    void onSwapchainRecreate();

private:
    void createSceneNodes();
    void drawSceneNodeRecursive(SceneNode* sceneNode);
    void sceneNodeDragDropSource(SceneNode* sceneNode);
    void sceneNodeDragDropTarget(SceneNode* sceneNode);

    void createDescriptorPool();
    void createDepthImages();
    void createViewProjUBOs();
    void createViewProjDescriptors();

    void createImguiImages();
    void createImguiRenderpass();
    void createImguiFramebuffers();
    void blitToSwapchainImage(VkCommandBuffer commandBuffer, uint32_t frameIndex, uint32_t swapchainImageIndex);

    void updateImgui();
    void renderImgui(VkCommandBuffer commandBuffer, uint32_t frameIndex);

    void updateUniformBuffers(uint32_t frameIndex);

private:
    VulkanRenderDevice* mRenderDevice;
    VulkanSwapchain* mSwapchain;
    VkDescriptorPool mDescriptorPool;
    VkSampleCountFlagBits mMsaaSampleCount;

    Camera mSceneCamera;

    VkDescriptorSetLayout mViewProjDSLayout;
    std::vector<VkDescriptorSet> mViewProjDS;
    std::vector<VulkanBuffer> mViewProjUBOs;

    VkRenderPass mImguiRenderpass;
    std::vector<VulkanImage> mImguiImages;
    std::vector<VkFramebuffer> mImguiFramebuffers;

    std::vector<VulkanImage> mDepthImages;

    SceneNode* mSceneRoot;
};

#endif //VULKANRENDERINGENGINE_RENDERER_HPP
