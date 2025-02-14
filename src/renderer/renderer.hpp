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

private:
    void executeClearRenderpass(VkCommandBuffer commandBuffer);
    void executePrepass(VkCommandBuffer commandBuffer);
    void executeResolveRenderpass(VkCommandBuffer commandBuffer);
    void executeGridRenderpass(VkCommandBuffer commandBuffer);
    void executeSsaoRenderpass(VkCommandBuffer commandBuffer);
    void executeShadingRenderpass(VkCommandBuffer commandBuffer);
    void executePostProcessingRenderpass(VkCommandBuffer commandBuffer);
    void setViewport(VkCommandBuffer commandBuffer);
    void renderModels(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t matDsIndex);

    void createColorTextures();
    void createDepthTextures();
    void createNormalTextures();
    void createSsaoTexture();
    void createPostProcessTexture();
    void createTextures();
    void createCameraDs();
    void updateCameraUBO();
    void createDisplayTexturesDsLayout();
    void createMaterialDsLayout();

    void createClearRenderPass();
    void createClearFramebuffer();

    void createPrepassRenderpass();
    void createPrepassFramebuffer();
    void createPrepassPipelineLayout();
    void createPrepassPipeline();

    void createResolveRenderpass();
    void createResolveFramebuffer();
    void createMultisampledNormalDepthDsLayout();
    void createResolvedNormalDepthDsLayout();
    void createMultisampledNormalDepthDs();
    void createResolvedNormalDepthDs();
    void createResolvePipelineLayout();
    void createResolvePipeline();

    void createColorDepthRenderpass();
    void createColorDepthFramebuffer();

    void createCubemapDsLayout();
    void createSkyboxPipelineLayout();
    void createSkyboxPipeline();

    void createGridDsLayout();
    void createGridPipelineLayout();
    void createGridPipeline();

    void createSsaoRenderpass();
    void createSsaoFramebuffer();
    void createSsaoPipelineLayout();
    void createSsaoPipeline();
    void createSsaoDsLayout();
    void createSsaoDs();

    void createShadingRenderpass();
    void createShadingFramebuffer();
    void createShadingPipelineLayout();
    void createShadingPipeline();
    void createResolvedColorDsLayout();
    void createResolvedColorDs();

    void createPostProcessingRenderpass();
    void createPostProcessingFramebuffer();
    void createPostProcessingPipelineLayout();
    void createPostProcessingPipeline();

private:
    const VulkanRenderDevice& mRenderDevice;
    SaveData& mSaveData;

    uint32_t mWidth;
    uint32_t mHeight;
    VkSampleCountFlagBits mSamples;

    // camera
    Camera mCamera;
    VulkanBuffer mCameraUBO;

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
    VkFramebuffer mClearFramebuffer{};
    VkFramebuffer mPrepassFramebuffer{};
    VkFramebuffer mResolveFramebuffer{};
    VkFramebuffer mColorDepthFramebuffer{};
    VkFramebuffer mSsaoFramebuffer{};
    VkFramebuffer mShadingFramebuffer{};
    VkFramebuffer mPostProcessingFramebuffer{};

    // pipeline layouts
    VkPipelineLayout mPrepassPipelineLayout{};
    VkPipelineLayout mResolvePipelineLayout{};
    VkPipelineLayout mSkyboxPipelineLayout{};
    VkPipelineLayout mGridPipelineLayout{};
    VkPipelineLayout mSsaoPipelineLayout{};
    VkPipelineLayout mShadingPipelineLayout{};
    VkPipelineLayout mPostProcessingPipelineLayout{};

    // pipelines
    VkPipeline mPrepassPipeline{};
    VkPipeline mResolvePipeline{};
    VkPipeline mSkyboxPipeline{};
    VkPipeline mGridPipeline{};
    VkPipeline mSsaoPipeline{};
    VkPipeline mShadingPipeline{};
    VkPipeline mPostProcessingPipeline{};

    // descriptor set layouts
    VkDescriptorSetLayout mCameraDsLayout{};
    VkDescriptorSetLayout mMultisampledNormalDepthDsLayout{};
    VkDescriptorSetLayout mResolvedNormalDepthDsLayout{};
    VkDescriptorSetLayout mResolvedColorDsLayout{};
    VkDescriptorSetLayout mCubemapDsLayout{};
    VkDescriptorSetLayout mGridDsLayout{};
    VkDescriptorSetLayout mSsaoDsLayout{};
    VkDescriptorSetLayout mDisplayTexturesDSLayout{};
    VkDescriptorSetLayout mMaterialsDsLayout{};

    // descriptor sets
    VkDescriptorSet mCameraDs{};
    VkDescriptorSet mMultisampledNormalDepthDs{};
    VkDescriptorSet mResolvedNormalDepthDs{};
    VkDescriptorSet mSsaoDs{};
    VkDescriptorSet mResolvedColorDs{};

    // models
    std::unordered_map<uuid32_t, std::shared_ptr<Model>> mModels;

    // async loading
    std::vector<std::future<std::shared_ptr<Model>>> mLoadedModelFutures;
    MainThreadTaskQueue mTaskQueue;

private:
    friend class Editor;
};

#endif //VULKANRENDERINGENGINE_RENDERER_HPP
