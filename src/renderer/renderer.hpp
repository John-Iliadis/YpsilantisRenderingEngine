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
#include "model.hpp"
#include "material.hpp"

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
    void fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t frameIndex, uint32_t imageIndex);
    void onSwapchainRecreate();

private:
    void createDescriptorPool();
    void createDepthImages();
    void createViewProjUBOs();
    void createViewProjDescriptors();

    void createImguiTextures();
    void createImguiRenderpass();
    void createImguiFramebuffers();

    void createFinalRenderPass();
    void createSwapchainImageFramebuffers();
    void createFinalDescriptorResources();
    void createFinalPipelineLayout();
    void createFinalPipeline();
    void updateFinalDescriptors();
    void renderToSwapchainImageFinal(VkCommandBuffer commandBuffer, uint32_t frameIndex, uint32_t imageIndex);

    void imguiMainMenuBar();
    void imguiAssetPanel();
    void imguiSceneNodeRecursive(SceneNode* sceneNode);
    void imguiSceneNodeDragDropSource(SceneNode* sceneNode);
    void imguiSceneNodeDragDropTarget(SceneNode* sceneNode);
    void imguiSceneGraph();
    void updateImgui();
    void renderImgui(VkCommandBuffer commandBuffer, uint32_t frameIndex);

    void updateUniformBuffers(uint32_t frameIndex);

private:
    VulkanRenderDevice* mRenderDevice;
    VulkanSwapchain* mSwapchain;
    VkDescriptorPool mDescriptorPool;
    VkSampleCountFlagBits mMsaaSampleCount;

    Camera mSceneCamera;
    SceneNode mSceneRoot;

    // scene resources
    std::vector<Model> mModels;
    std::vector<std::shared_ptr<InstancedMesh>> mMeshes;
    std::vector<std::shared_ptr<NamedMaterial>> mNamedMaterials;
    std::vector<GpuMaterial> mMaterials;
    std::vector<NamedTexture> mTextures;

    // final renderpass for writing imgui image to swapchain image.
    VkRenderPass mFinalRenderPass;
    VkPipelineLayout mFinalPipelineLayout;
    VkPipeline mFinalPipeline;
    VkDescriptorSetLayout mFinalDSLayout;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> mFinalDS;
    std::vector<VkFramebuffer> mSwapchainImageFramebuffers;

    // view projection descriptors and buffer
    VkDescriptorSetLayout mViewProjDSLayout;
    std::array<VkDescriptorSet, MAX_FRAMES_IN_FLIGHT> mViewProjDS;
    std::array<VulkanBuffer, MAX_FRAMES_IN_FLIGHT> mViewProjUBOs;

    // imgui renderpass
    VkRenderPass mImguiRenderpass;
    std::array<VulkanTexture, MAX_FRAMES_IN_FLIGHT> mImguiTextures;
    std::array<VkFramebuffer, MAX_FRAMES_IN_FLIGHT> mImguiFramebuffers;

    std::array<VulkanImage, MAX_FRAMES_IN_FLIGHT> mDepthImages;
};

#endif //VULKANRENDERINGENGINE_RENDERER_HPP
