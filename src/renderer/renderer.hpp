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
#include "loaders/cubemap_loader.hpp"
#include "../scene_graph/scene_graph.hpp"
#include "camera.hpp"
#include "model.hpp"
#include "model_importer.hpp"
#include "lights.hpp"

constexpr uint32_t InitialViewportWidth = 1000;
constexpr uint32_t InitialViewportHeight = 700;
constexpr uint32_t MaxDirLights = 1024;
constexpr uint32_t MaxPointLights = 1024;
constexpr uint32_t MaxSpotLights = 1024;

struct TransparentNode;
struct LightIconRenderData;

// todo: remove light icon depth buffer if not needed
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

    void addDirLight(uuid32_t id, const DirectionalLight& light);
    void addPointLight(uuid32_t id, const PointLight& light);
    void addSpotLight(uuid32_t id, const SpotLight& light);
    DirectionalLight& getDirLight(uuid32_t id);
    PointLight& getPointLight(uuid32_t id);
    SpotLight& getSpotLight(uuid32_t id);
    void updateDirLight(uuid32_t id);
    void updateSpotLight(uuid32_t id);
    void updatePointLight(uuid32_t id);
    void deleteDirLight(uuid32_t id);
    void deletePointLight(uuid32_t id);
    void deleteSpotLight(uuid32_t id);

private:
    void executePrepass(VkCommandBuffer commandBuffer);
    void executeSkyboxRenderpass(VkCommandBuffer commandBuffer);
    void executeSsaoRenderpass(VkCommandBuffer commandBuffer);
    void executeForwardRenderpass(VkCommandBuffer commandBuffer);
    void executeGridRenderpass(VkCommandBuffer commandBuffer);
    void executePostProcessingRenderpass(VkCommandBuffer commandBuffer);
    void executeLightIconRenderpass(VkCommandBuffer commandBuffer);
    void setViewport(VkCommandBuffer commandBuffer);
    void renderModels(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t matDsIndex, bool opaque);
    void updateCameraUBO();
    void getLightIconRenderData();
    void bindTexture(VkCommandBuffer commandBuffer,
                     VkPipelineLayout pipelineLayout,
                     const VulkanTexture& texture,
                     uint32_t binding,
                     uint32_t set);

    void createColorTexture32F();
    void createColorTexture8U();
    void createDepthTexture();
    void createNormalTexture();
    void createSsaoTexture();
    void createIconDepthTexture();

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

    void createLightBuffers();
    void createLightIconTextures();
    void createLightIconTextureDsLayout();
    void createLightIconRenderpass();
    void createLightIconFramebuffer();
    void createLightIconPipeline();

    void loadSkybox();

private:
    const VulkanRenderDevice& mRenderDevice;
    SaveData& mSaveData;
    SceneGraph mSceneGraph;

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

    // render targets
    VulkanTexture mTransparencyTexture;
    VulkanTexture mColorTexture32F;
    VulkanTexture mColorTexture8U;
    VulkanTexture mDepthTexture;
    VulkanTexture mNormalTexture;
    VulkanTexture mSsaoTexture;
    VulkanTexture mIconDepthTexture;
    VulkanTexture mSkyboxTexture;

    // render passes
    VkRenderPass mPrepassRenderpass{};
    VkRenderPass mSkyboxRenderpass{};
    VkRenderPass mSsaoRenderpass{};
    VkRenderPass mForwardRenderpass{};
    VkRenderPass mGridRenderpass{};
    VkRenderPass mPostProcessingRenderpass{};
    VkRenderPass mLightIconRenderpass{};

    // framebuffers
    VkFramebuffer mPrepassFramebuffer{};
    VkFramebuffer mSkyboxFramebuffer{};
    VkFramebuffer mForwardPassFramebuffer{};
    VkFramebuffer mSsaoFramebuffer{};
    VkFramebuffer mGridFramebuffer{};
    VkFramebuffer mPostProcessingFramebuffer{};
    VkFramebuffer mLightIconFramebuffer{};

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
    VulkanGraphicsPipeline mLightIconPipeline;

    // descriptor set layouts
    VulkanDsLayout mSingleImageDsLayout;
    VulkanDsLayout mCameraRenderDataDsLayout;
    VulkanDsLayout mMaterialsDsLayout;
    VulkanDsLayout mDepthNormalDsLayout;
    VulkanDsLayout mIconTextureDsLayout;

    // descriptor sets
    VkDescriptorSet mCameraDs{};
    VkDescriptorSet mDepthNormalDs{};
    VkDescriptorSet mSsaoDs{};
    VkDescriptorSet mDepthDs{};
    VkDescriptorSet mSkyboxDs{};
    VkDescriptorSet mColor32FDs{};
    VkDescriptorSet mColor8UDs{};

    // models
    std::unordered_map<uuid32_t, std::shared_ptr<Model>> mModels;

    // lights
    std::vector<DirectionalLight> mDirectionalLights;
    std::vector<PointLight> mPointLights;
    std::vector<SpotLight> mSpotLights;

    std::unordered_map<uuid32_t, index_t> mUuidToDirLightIndex;
    std::unordered_map<uuid32_t, index_t> mUuidToPointLightIndex;
    std::unordered_map<uuid32_t, index_t> mUuidToSpotLightIndex;

    VulkanBuffer mDirLightSSBO;
    VulkanBuffer mPointLightSSBO;
    VulkanBuffer mSpotLightSSBO;

    VulkanTexture mDirLightIcon;
    VulkanTexture mPointLightIcon;
    VulkanTexture mSpotLightIcon;

    // render data
    glm::vec4 mClearColor = glm::vec4(0.251f, 0.235f, 0.235f, 1.f);
    uint32_t mOitLinkedListLength = 8;
    std::vector<LightIconRenderData> mLightIconRenderData;

    struct SkyboxData
    {
        alignas(4) int32_t flipX = false;
        alignas(4) int32_t flipY = false;
        alignas(4) int32_t flipZ = false;
    } mSkyboxData;

    struct GridData
    {
        alignas(16) glm::vec4 thinLineColor = glm::vec4(0.5f, 0.5f, 0.5f, 80.f / 255.f);
        alignas(16) glm::vec4 thickLineColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.f);
    } mGridData;

    bool mRenderSkybox = true;
    bool mSsaoOn = false;
    bool mOitOn = true;
    bool mRenderGrid = true;
    bool mRenderAxisGizmo = true;

private:
    friend class Editor;
};

struct TransparentNode
{
    alignas(16) glm::vec4 color;
    alignas(4) float depth;
    alignas(4) uint32_t next;
};

struct LightIconRenderData
{
    glm::vec3 pos;
    VulkanTexture* icon;
};

#include "renderer.inl"

#endif //VULKANRENDERINGENGINE_RENDERER_HPP
