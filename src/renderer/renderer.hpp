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
#include "model.hpp"
#include "model_importer.hpp"
#include "loaders/cubemap_loader.hpp"

constexpr uint32_t InitialViewportWidth = 1000;
constexpr uint32_t InitialViewportHeight = 700;

struct TransparentNode;

class Renderer : public SubscriberSNS
{
public:
    Renderer(const VulkanRenderDevice& renderDevice, SaveData& saveData);
    ~Renderer();

    void update();
    void render(VkCommandBuffer commandBuffer);

    void importModel(const ModelImportData& importData);
    void importSkybox(const std::array<std::string, 6>& paths);
    void deleteModel(uuid32_t id);
    void resize(uint32_t width, uint32_t height);
    void notify(const Message &message) override;

private:
    void executePrepass(VkCommandBuffer commandBuffer);
    void executeSkyboxRenderpass(VkCommandBuffer commandBuffer);
    void executeSsaoRenderpass(VkCommandBuffer commandBuffer);
    void executeForwardRenderpass(VkCommandBuffer commandBuffer);
    void executeGridRenderpass(VkCommandBuffer commandBuffer);
    void executePostProcessingRenderpass(VkCommandBuffer commandBuffer);
    void setViewport(VkCommandBuffer commandBuffer);
    void renderModels(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t matDsIndex, bool opaque);
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

    void createPrepassRenderpass();
    void createPrepassFramebuffer();
    void createPrepassPipeline();

    void createSkyboxRenderpass();
    void createSkyboxFramebuffer();
    void createSkyboxPipeline();

    void createSsaoRenderpass();
    void createSsaoFramebuffer();
    void createSsaoPipeline();

    void createOitTextures();
    void createOitBuffers();
    void createOitResourcesDs();
    void updateOitResourcesDs();
    void createForwardRenderpass();
    void createForwardFramebuffer();
    void createOitTransparentCollectionPipeline();
    void createOitTransparencyResolutionPipeline();
    void createOpaqueForwardPassPipeline();
    void createForwardPassBlendPipeline();

    void createGridRenderpass();
    void createGridFramebuffer();
    void createGridPipeline();

    void createPostProcessingRenderpass();
    void createPostProcessingFramebuffer();
    void createPostProcessingPipeline();

    void loadSkybox();

private:
    const VulkanRenderDevice& mRenderDevice;
    SaveData& mSaveData;

    uint32_t mWidth;
    uint32_t mHeight;

    // camera
    Camera mCamera;
    VulkanBuffer mCameraUBO;

    // oit
    VulkanImage mTransparentNodeIndexStorageImage;
    VulkanBuffer mOitLinkedListInfoSSBO;
    VulkanBuffer mOitLinkedListSSBO;
    VulkanDsLayout mOitResourcesDsLayout;
    VkDescriptorSet mOitResourcesDs;
    VulkanDsLayout mSingleInputAttachmentDsLayout;
    VkDescriptorSet mTransparentTexInputAttachmentDs;
    uint32_t mOitLinkedListLength;

    // render targets
    VulkanTexture mTransparencyTexture;
    VulkanTexture mColorTexture;
    VulkanTexture mDepthTexture;
    VulkanTexture mNormalTexture;
    VulkanTexture mPostProcessingTexture;
    VulkanTexture mSsaoTexture;
    VulkanTexture mSkyboxTexture;

    // render passes
    VkRenderPass mPrepassRenderpass{};
    VkRenderPass mSkyboxRenderpass{};
    VkRenderPass mSsaoRenderpass{};
    VkRenderPass mForwardRenderpass{};
    VkRenderPass mGridRenderpass{};
    VkRenderPass mPostProcessingRenderpass{};

    // framebuffers
    VkFramebuffer mPrepassFramebuffer{};
    VkFramebuffer mSkyboxFramebuffer{};
    VkFramebuffer mForwardPassFramebuffer{};
    VkFramebuffer mSsaoFramebuffer{};
    VkFramebuffer mGridFramebuffer{};
    VkFramebuffer mPostProcessingFramebuffer{};

    // pipelines
    VulkanGraphicsPipeline mPrepassPipeline;
    VulkanGraphicsPipeline mSkyboxPipeline;
    VulkanGraphicsPipeline mGridPipeline;
    VulkanGraphicsPipeline mSsaoPipeline;
    VulkanGraphicsPipeline mTransparentCollectionPipeline;
    VulkanGraphicsPipeline mTransparencyResolutionPipeline;
    VulkanGraphicsPipeline mOpaqueForwardPassPipeline;
    VulkanGraphicsPipeline mForwardPassBlendPipeline;
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
    VkDescriptorSet mSkyboxDs{};
    VkDescriptorSet mColorDs{};
    VkDescriptorSet mPostProcessingDs{};

    // models
    std::unordered_map<uuid32_t, std::shared_ptr<Model>> mModels;

    struct SkyboxData
    {
        alignas(4) int32_t flipX = false;
        alignas(4) int32_t flipY = false;
        alignas(4) int32_t flipZ = false;
    } mSkyboxData;

    // grid data
    struct GridData
    {
        alignas(16) glm::vec4 thinLineColor = glm::vec4(0.5f, 0.5f, 0.5f, 80.f / 255.f);
        alignas(16) glm::vec4 thickLineColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.f);
        alignas(4) float cellSize = 0.5f;
        alignas(4) float minPixelsBetweenCells = 0.7f;
    } mGridData;

    bool mRenderSkybox = true;
    bool mSsaoOn = false;
    bool mRenderGrid = true;

private:
    friend class Editor;
};

struct TransparentNode
{
    alignas(16) glm::vec4 color;
    alignas(4) float depth;
    alignas(4) uint32_t next;
};

#endif //VULKANRENDERINGENGINE_RENDERER_HPP
