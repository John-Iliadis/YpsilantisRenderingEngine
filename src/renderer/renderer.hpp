//
// Created by Gianni on 4/01/2025.
//

#ifndef VULKANRENDERINGENGINE_RENDERER_HPP
#define VULKANRENDERINGENGINE_RENDERER_HPP

#include "../utils/utils.hpp"
#include "../utils/main_thread_task_queue.hpp"
#include "../app/save_data.hpp"
#include "../vk/vulkan_pipeline.hpp"
#include "camera.hpp"
#include "model_importer.hpp"
#include "model.hpp"

constexpr uint32_t InitialViewportWidth = 1000;
constexpr uint32_t InitialViewportHeight = 700;

class Renderer
{
public:
    Renderer(const VulkanRenderDevice& renderDevice, SaveData& saveData);
    ~Renderer();

    void render();

    void importModel(const std::filesystem::path& path);
    void deleteModel(uuid32_t id);
    void resize(uint32_t width, uint32_t height);

    void processMainThreadTasks();
    void releaseResources();
    void updateCameraUBO();

private:
    void createColorTextures();
    void createDepthTextures();
    void createNormalTextures();
    void createSsaoTexture();
    void createPostProcessTexture();
    void createTextures();

    void createCameraDs();
    void createDisplayTexturesDsLayout();
    void createMaterialDsLayout();

    void createClearRenderPass();

    void createPrepassRenderpass();
    void createPrepassFramebuffer();
    void createPrepassPipelineLayout();
    void createPrepassPipeline();

    void createResolveRenderpass();
    void createResolveFramebuffer();
    void createResolvePipeline

    void createColorDepthRenderpass();
    void createColorDepthFramebuffer();

    void createSsaoRenderpass();
    void createSsaoFramebuffer();

    void createShadingRenderpass();
    void createShadingFramebuffer();

    void createPostProcessingRenderpass();
    void createPostProcessingFramebuffer();

private:
    const VulkanRenderDevice& mRenderDevice;
    SaveData& mSaveData;

    uint32_t mWidth;
    uint32_t mHeight;
    VkSampleCountFlagBits mSamples;

    // camera
    Camera mCamera;
    VulkanBuffer mCameraUBO;
    VkDescriptorSetLayout mCameraDsLayout{};
    VkDescriptorSet mCameraDs{};

    // textures
    VulkanTexture mColorTexture;
    VulkanTexture mResolvedColorTexture;
    VulkanTexture mDepthTexture;
    VulkanTexture mResolvedDepthTexture;
    VulkanTexture mNormalTexture;
    VulkanTexture mResolvedNormalTexture;
    VulkanTexture mPostProcessTexture;
    VulkanTexture mSkybox;
    VulkanTexture mSsaoTexture;

    // render passes
    VkRenderPass mClearRenderpass{};
    VkRenderPass mPrepassRenderpass{};
    VkRenderPass mResolveRenderpass{};
    VkRenderPass mColorDepthRenderpass{};
    VkRenderPass mSsaoRenderpass{};
    VkRenderPass mShadingRenderpass{};
    VkRenderPass mPostProcessingRenderpass{};

    // framebuffers
    VkFramebuffer mPrepassFramebuffer{};
    VkFramebuffer mResolveFramebuffer{};
    VkFramebuffer mColorDepthFramebuffer{};
    VkFramebuffer mSsaoFramebuffer{};
    VkFramebuffer mShadingFramebuffer{};
    VkFramebuffer mPostProcessingFramebuffer{};

    // pipeline layouts
    VkPipelineLayout mPrepassPipelineLayout{};

    // pipelines
    VkPipeline mPrepassPipeline{};

    // descriptor set layouts
    VkDescriptorSetLayout mDisplayTexturesDSLayout{};
    VkDescriptorSetLayout mMaterialsDsLayout{};

    // models
    std::unordered_map<uuid32_t, std::shared_ptr<Model>> mModels;
    uuid32_t mDeferDeleteModel{};

    // async loading
    std::vector<std::future<std::shared_ptr<Model>>> mLoadedModelFutures;
    MainThreadTaskQueue mTaskQueue;

private:
    friend class Editor;
};

#endif //VULKANRENDERINGENGINE_RENDERER_HPP
