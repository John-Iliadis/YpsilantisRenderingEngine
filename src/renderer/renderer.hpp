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

    void render();

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
    void executeTempColorTransitionRenderpass(VkCommandBuffer commandBuffer);
    void executePostProcessingRenderpass(VkCommandBuffer commandBuffer);
    void setViewport(VkCommandBuffer commandBuffer);
    void renderModels(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t matDsIndex);
    void updateCameraUBO();
    void beginRenderpassTimestamp(VkCommandBuffer commandBuffer, uint32_t index);
    void endRenderpassTimestamp(VkCommandBuffer commandBuffer, uint32_t index);
    void getRenderpassDurations();
    double getRenderpassDurationMs(uint32_t index);

    void createColorTexture();
    void createDepthTexture();
    void createNormalTexture();
    void createSsaoTexture();
    void createPostProcessTexture();

    void createQueryPool();
    void createSingleImageDsLayout();
    void createCameraRenderDataDsLayout();
    void createMaterialsDsLayout();
    void createDepthNormalInputDsLayout();

    void createSingleImageDs(VkDescriptorSet& ds, const VulkanTexture& texture, const char* name);
    void createCameraDs();
    void createSingleImageDescriptorSets();
    void createDepthNormalInputDs();

    void createClearRenderPass();
    void createClearFramebuffer();

    void createPrepassRenderpass();
    void createPrepassFramebuffer();
    void createPrepassPipelineLayout();
    void createPrepassPipeline();

    void createColorDepthRenderpass();
    void createColorDepthFramebuffer();

    void createSkyboxPipelineLayout();
    void createSkyboxPipeline();

    void createSsaoRenderpass();
    void createSsaoFramebuffer();
    void createSsaoPipelineLayout();
    void createSsaoPipeline();

    void createShadingPipelineLayout();
    void createShadingPipeline();

    void createGridPipelineLayout();
    void createGridPipeline();

    void createTempColorTransitionRenderpass();
    void createTempColorTransitionFramebuffer();

    void createPostProcessingRenderpass();
    void createPostProcessingFramebuffer();
    void createPostProcessingPipelineLayout();
    void createPostProcessingPipeline();

private:
    const VulkanRenderDevice& mRenderDevice;
    SaveData& mSaveData;

    uint32_t mWidth;
    uint32_t mHeight;

    // camera
    Camera mCamera;
    VulkanBuffer mCameraUBO;

    // textures
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
    VkRenderPass mTempColorTransitionRenderpass{};
    VkRenderPass mPostProcessingRenderpass{};

    // framebuffers
    VkFramebuffer mClearFramebuffer{};
    VkFramebuffer mPrepassFramebuffer{};
    VkFramebuffer mColorDepthFramebuffer{};
    VkFramebuffer mSsaoFramebuffer{};
    VkFramebuffer mTempColorTransitionFramebuffer{};
    VkFramebuffer mPostProcessingFramebuffer{};

    // pipeline layouts
    VkPipelineLayout mPrepassPipelineLayout{};
    VkPipelineLayout mSkyboxPipelineLayout{};
    VkPipelineLayout mGridPipelineLayout{};
    VkPipelineLayout mSsaoPipelineLayout{};
    VkPipelineLayout mShadingPipelineLayout{};
    VkPipelineLayout mPostProcessingPipelineLayout{};

    // pipelines
    VkPipeline mPrepassPipeline{};
    VkPipeline mSkyboxPipeline{};
    VkPipeline mGridPipeline{};
    VkPipeline mSsaoPipeline{};
    VkPipeline mShadingPipeline{};
    VkPipeline mPostProcessingPipeline{};

    // descriptor set layouts
    VkDescriptorSetLayout mSingleImageDsLayout{};
    VkDescriptorSetLayout mCameraRenderDataDsLayout{};
    VkDescriptorSetLayout mMaterialsDsLayout{};
    VkDescriptorSetLayout mDepthNormalInputDsLayout{};

    // descriptor sets
    VkDescriptorSet mCameraDs{};
    VkDescriptorSet mDepthNormalInputDs{};
    VkDescriptorSet mSsaoDs{};
    VkDescriptorSet mDepthDs{};
    VkDescriptorSet mColorDs{};
    VkDescriptorSet mPostProcessingDs{};

    // monitoring
    VkQueryPool mRenderpassQueryPool;
    struct RenderpassTimes {
        double clearRenderpassDurMs;
        double prepassRenderpassDurMs;
        double skyboxRenderpassDurMs;
        double ssaoRenderpassDurMs;
        double shadingRenderpassDurMs;
        double gridRenderpassDurMs;
        double tempColorTransitionDurMs;
        double postProcessingRenderpassDurMs;
    } mRenderpassTimes;

    // models
    std::unordered_map<uuid32_t, std::shared_ptr<Model>> mModels;

    // async loading
    std::vector<std::future<std::shared_ptr<Model>>> mLoadedModelFutures;
    MainThreadTaskQueue mTaskQueue;

    struct GridData
    {
        alignas(16) glm::vec4 thinLineColor = glm::vec4(0.5, 0.5, 0.5, 1.f);
        alignas(16) glm::vec4 thickLineColor = glm::vec4(0.f, 0.f, 0.f, 1.f);
        alignas(4) float cellSize = 0.025f;
        alignas(4) float minPixelsBetweenCells = 1.2f;
    } mGridData;

    bool mSkyboxLoaded = false;
    bool mRenderSkybox = false;
    bool mRenderGrid = true;

private:
    friend class Editor;
};

#endif //VULKANRENDERINGENGINE_RENDERER_HPP
