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
#include "importer.hpp"
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
    void fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t frameIndex, uint32_t swapchainImageIndex);
    void onSwapchainRecreate();

private:
    void createDescriptorPool();
    void createDepthImages();
    void createViewProjUBOs();
    void createViewProjDescriptors();

    void createImguiImages();
    void createImguiRenderpass();
    void createImguiFramebuffers();
    void blitToSwapchainImage(VkCommandBuffer commandBuffer, uint32_t frameIndex, uint32_t swapchainImageIndex);

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

    std::unordered_map<std::string_view, Model> mModels;
    std::unordered_map<std::string_view, Mesh> mMeshes;
    std::unordered_map<std::string_view, size_t> mMaterialsMapped;
    std::unordered_map<std::string_view, size_t> mTexturesMapped;
    std::vector<Material> mMaterials; // matches SSBO
    std::vector<Texture> mTextures; // matches descriptor array
    std::unordered_set<std::string> mLoadedModels;

    // todo: make std arrays
    VkDescriptorSetLayout mViewProjDSLayout;
    std::vector<VkDescriptorSet> mViewProjDS;
    std::vector<VulkanBuffer> mViewProjUBOs;

    VkRenderPass mImguiRenderpass;
    std::vector<VulkanImage> mImguiImages;
    std::vector<VkFramebuffer> mImguiFramebuffers;

    std::vector<VulkanImage> mDepthImages;
};

#endif //VULKANRENDERINGENGINE_RENDERER_HPP
