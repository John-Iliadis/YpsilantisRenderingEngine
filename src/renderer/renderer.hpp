//
// Created by Gianni on 4/01/2025.
//

#ifndef VULKANRENDERINGENGINE_RENDERER_HPP
#define VULKANRENDERINGENGINE_RENDERER_HPP

#include "../utils/utils.hpp"
#include "../utils/timer.hpp"
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

    void render(VkCommandBuffer commandBuffer);

    void importModel(const std::filesystem::path& path);
    void deleteModel(uuid32_t id);

    void resize(uint32_t width, uint32_t height);

    void processMainThreadTasks();

private:
    void executeClearRenderpass(VkCommandBuffer commandBuffer);
    void executePrepass(VkCommandBuffer commandBuffer);
    void executeSkyboxRenderpass(VkCommandBuffer commandBuffer);
    void executeSsaoRenderpass(VkCommandBuffer commandBuffer);
    void executeShadingRenderpass(VkCommandBuffer commandBuffer);
    void executeGridRenderpass(VkCommandBuffer commandBuffer);
    void executePostProcessingRenderpass(VkCommandBuffer commandBuffer);
    void setViewport(VkCommandBuffer commandBuffer);
    void renderModels(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t matDsIndex);
    void updateCameraUBO();

    void createColorTexture();
    void createDepthTexture();
    void createNormalTexture();
    void createSsaoTexture();
    void createPostProcessTexture();

    void createSingleImageDsLayout();
    void createCameraRenderDataDsLayout();
    void createMaterialsDsLayout();
    void createDepthNormalInputDsLayout();

    void createSingleImageDs(VkDescriptorSet& ds, const VulkanTexture& texture, const char* name);
    void createCameraDs();
    void createSingleImageDescriptorSets();
    void createDepthNormalDs();

    void createClearRenderPass();
    void createClearFramebuffer();

    void createPrepassRenderpass();
    void createPrepassFramebuffer();
    void createPrepassPipeline();

    void createColorDepthRenderpass();
    void createColorDepthFramebuffer();

    void createSkyboxPipeline();

    void createSsaoRenderpass();
    void createSsaoFramebuffer();
    void createSsaoPipeline();

    void createShadingPipeline();
    void createGridPipeline();

    void createPostProcessingRenderpass();
    void createPostProcessingFramebuffer();
    void createPostProcessingPipeline();

private:
    const VulkanRenderDevice& mRenderDevice;
    SaveData& mSaveData;

    uint32_t mWidth;
    uint32_t mHeight;

    // camera
    Camera mCamera;
    VulkanBuffer mCameraUBO;

    // render targets
    VulkanTexture mColorTexture;
    VulkanTexture mDepthTexture;
    VulkanTexture mNormalTexture;
    VulkanTexture mPostProcessingTexture;
    VulkanTexture mSsaoTexture;
    VulkanTexture mSkyboxTexture;

    // render passes
    VkRenderPass mClearRenderpass{};
    VkRenderPass mPrepassRenderpass{};
    VkRenderPass mColorDepthRenderpass{};
    VkRenderPass mSsaoRenderpass{};
    VkRenderPass mPostProcessingRenderpass{};

    // framebuffers
    VkFramebuffer mClearFramebuffer{};
    VkFramebuffer mPrepassFramebuffer{};
    VkFramebuffer mColorDepthFramebuffer{};
    VkFramebuffer mSsaoFramebuffer{};
    VkFramebuffer mPostProcessingFramebuffer{};

    // pipelines
    VulkanGraphicsPipeline mPrepassPipeline;
    VulkanGraphicsPipeline mSkyboxPipeline;
    VulkanGraphicsPipeline mGridPipeline;
    VulkanGraphicsPipeline mSsaoPipeline;
    VulkanGraphicsPipeline mShadingPipeline;
    VulkanGraphicsPipeline mPostProcessingPipeline;

    // descriptor set layouts
    VulkanDsLayout mSingleImageDsLayout;
    VulkanDsLayout mCameraRenderDataDsLayout;
    VulkanDsLayout mMaterialsDsLayout;
    VulkanDsLayout mDepthNormalDsLayout;

    // descriptor sets
    VkDescriptorSet mCameraDs{};
    VkDescriptorSet mDepthNormalDs{};
    VkDescriptorSet mSsaoDs{};
    VkDescriptorSet mDepthDs{};
    VkDescriptorSet mColorDs{};
    VkDescriptorSet mPostProcessingDs{};

    // models
    std::unordered_map<uuid32_t, std::shared_ptr<Model>> mModels;

    // async loading
    std::vector<std::future<std::shared_ptr<Model>>> mLoadedModelFutures;
    MainThreadTaskQueue mTaskQueue;

    struct GridData
    {
        alignas(16) glm::vec4 thinLineColor = glm::vec4(0.5, 0.5, 0.5, 1.f);
        alignas(16) glm::vec4 thickLineColor = glm::vec4(0.f, 0.f, 0.f, 1.f);
        alignas(4) float cellSize = 0.5f;
        alignas(4) float minPixelsBetweenCells = 1.2f;
    } mGridData;

    bool mSkyboxLoaded = false;
    bool mRenderSkybox = false;
    bool mRenderGrid = true;

private:
    friend class Editor;
};

#endif //VULKANRENDERINGENGINE_RENDERER_HPP
