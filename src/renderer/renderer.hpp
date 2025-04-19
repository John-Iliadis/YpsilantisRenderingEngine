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
constexpr uint32_t MaxSsaoKernelSamples = 128;
constexpr uint32_t SsaoNoiseTextureSize = 4;
constexpr uint32_t PerClusterCapacity = 32;

struct TransparentNode;
struct LightIconRenderData;
struct Cluster;
enum class Tonemap;

class Renderer : public SubscriberSNS
{
public:
    Renderer(const VulkanRenderDevice& renderDevice, SaveData& saveData);
    ~Renderer();

    void update();
    void render(VkCommandBuffer commandBuffer);

    void importModel(const ModelImportData& importData);
    void importEnvMap(std::string path);
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
    void executeSsaoBlurRenderpass(VkCommandBuffer commandBuffer);
    void executeGenFrustumClustersRenderpass(VkCommandBuffer commandBuffer);
    void executeAssignLightsToClustersRenderpass(VkCommandBuffer commandBuffer);
    void executeForwardRenderpass(VkCommandBuffer commandBuffer);
    void executeBloomRenderpass(VkCommandBuffer commandBuffer);
    void executePostProcessingRenderpass(VkCommandBuffer commandBuffer);
    void executeGridRenderpass(VkCommandBuffer commandBuffer);
    void executeLightIconRenderpass(VkCommandBuffer commandBuffer);
    void setViewport(VkCommandBuffer commandBuffer);
    void renderOpaque(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t matDsIndex);
    void renderTransparent(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t matDsIndex);
    void renderAll(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t matDsIndex);
    void updateCameraUBO();
    void getLightIconRenderData();
    void bindTexture(VkCommandBuffer commandBuffer,
                     VkPipelineLayout pipelineLayout,
                     const VulkanTexture& texture,
                     uint32_t binding,
                     uint32_t set);

    void updateSceneGraph();
    void updateGraphNode(NodeType type, GraphNode* node);
    void updateEmptyNode(GraphNode* node);
    void updateMeshNode(GraphNode* node);
    void updateDirLightNode(GraphNode* node);
    void updatePointLightNode(GraphNode* node);
    void updateSpotLightNode(GraphNode* node);

    void createColorTexture32F();
    void createColorTexture8U();
    void createDepthTexture();
    void createPosTexture();
    void createNormalTexture();
    void createSsaoTextures();

    void createSingleImageDsLayout();
    void createCameraRenderDataDsLayout();
    void createMaterialsDsLayout();
    void createSsaoDsLayout();
    void createOitResourcesDsLayout();
    void createSingleInputAttachmentDsLayout();
    void createLightsDsLayout();
    void createFrustumClusterGenDsLayout();
    void createAssignLightsToClustersDsLayout();
    void createForwardShadingDsLayout();
    void createPostProcessingDsLayout();

    void createPrepassRenderpass();
    void createPrepassFramebuffer();
    void createPrepassPipeline();

    void createVolumeClusterSSBO();
    void createFrustumClusterGenPipelineLayout();
    void createFrustumClusterGenPipeline();
    void createAssignLightsToClustersPipelineLayout();
    void createAssignLightsToClustersPipeline();

    void createSkyboxRenderpass();
    void createSkyboxFramebuffer();
    void createSkyboxPipeline();

    void createSsaoKernel(uint32_t sampleCount);
    void createSsaoKernelSSBO();
    void updateSsaoKernelSSBO();
    void createSsaoNoiseTexture();
    void createSsaoRenderpass();
    void createSsaoFramebuffer();
    void createSsaoBlurRenderpass();
    void createSsaoBlurFramebuffer();
    void createSsaoPipeline();
    void createSsaoBlurPipeline();

    void createOitTextures();
    void createOitBuffers();
    void createOitResourcesDs();
    void updateOitResourcesDs();
    void createForwardRenderpass();
    void createForwardFramebuffer();
    void createOpaqueForwardPassPipeline();
    void createOitTransparentCollectionPipeline();
    void createOitTransparencyResolutionPipeline();
    void createForwardPassBlendPipeline();

    void createPostProcessingRenderpass();
    void createPostProcessingFramebuffer();
    void createPostProcessingPipeline();

    void createGridRenderpass();
    void createGridFramebuffer();
    void createGridPipeline();

    void createLightBuffers();
    void createLightIconTextures();
    void createLightIconTextureDsLayout();
    void createLightIconRenderpass();
    void createLightIconFramebuffer();
    void createLightIconPipeline();

    void createGizmoIconResources();

    void createIrradianceMap();
    void createPrefilterMap();
    void createBrdfLut();
    void createEnvMapViewsUBO();
    void createIrradianceViewsUBO();
    void createCubemapConvertRenderpass();
    void createIrradianceConvolutionRenderpass();
    void createPrefilterRenderpass();
    void createBrdfLutRenderpass();
    void createCubemapConvertDsLayout();
    void createIrradianceConvolutionDsLayout();
    void createCubemapConvertPipeline();
    void createConvolutionPipeline();
    void createPrefilterPipeline();
    void createBrdfLutPipeline();
    void createIrradianceConvolutionFramebuffer();
    void createPrefilterFramebuffers();
    void createBrdfLutFramebuffer();

    void createEquirectangularTexture(const std::string& path);
    void createEnvMap();
    void createCubemapConvertFramebuffer();
    void createCubemapConvertDs();
    void createIrradianceConvolutionDs();

    void executeCubemapConvertRenderpass();
    void executeIrradianceConvolutionRenderpass();
    void executePrefilterRenderpasses();
    void executeBrdfLutRenderpass();

    void createBloomMipChain();
    void createCaptureBrightPixelsRenderpass();
    void createCaptureBrightPixelsFramebuffer();
    void createCaptureBrightPixelsPipeline();
    void createBloomDownsampleRenderpass();
    void createBloomDownsampleFramebuffers();
    void createBloomDownsamplePipeline();
    void createBloomUpsampleRenderpass();
    void createBloomUpsampleFramebuffers();
    void createBloomUpsamplePipeline();
    void createBloomMipChainDs();

    void createSingleImageDs(VkDescriptorSet& ds, const VulkanTexture& texture, const char* name);
    void createCameraDs();
    void createSingleImageDescriptorSets();
    void createSsaoDs();
    void createColor32FInputDs();
    void createLightsDs();
    void createFrustumClusterGenDs();
    void createAssignLightsToClustersDs();
    void createSkyboxDs();
    void createForwardShadingDs();
    void updateForwardShadingDs();
    void createPostProcessingDs();

private:
    const VulkanRenderDevice& mRenderDevice;
    std::shared_ptr<SceneGraph> mSceneGraph;
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
    VkDescriptorSet mOitResourcesDs;
    VkDescriptorSet mTransparentTexInputAttachmentDs;

    // render targets
    VulkanTexture mTransparencyTexture;
    VulkanTexture mColorTexture32F;
    VulkanTexture mColorTexture8U;
    VulkanTexture mDepthTexture;
    VulkanTexture mPosTexture;
    VulkanTexture mNormalTexture;
    VulkanTexture mSsaoTexture;
    VulkanTexture mSsaoBlurTexture;

    // render passes
    VkRenderPass mPrepassRenderpass{};
    VkRenderPass mSkyboxRenderpass{};
    VkRenderPass mSsaoRenderpass{};
    VkRenderPass mSsaoBlurRenderpass{};
    VkRenderPass mForwardRenderpass{};
    VkRenderPass mGridRenderpass{};
    VkRenderPass mPostProcessingRenderpass{};
    VkRenderPass mLightIconRenderpass{};

    // framebuffers
    VkFramebuffer mPrepassFramebuffer{};
    VkFramebuffer mSkyboxFramebuffer{};
    VkFramebuffer mForwardPassFramebuffer{};
    VkFramebuffer mSsaoFramebuffer{};
    VkFramebuffer mSsaoBlurFramebuffer{};
    VkFramebuffer mGridFramebuffer{};
    VkFramebuffer mPostProcessingFramebuffer{};
    VkFramebuffer mLightIconFramebuffer{};

    // graphics pipelines
    VulkanGraphicsPipeline mPrepassPipeline;
    VulkanGraphicsPipeline mSkyboxPipeline;
    VulkanGraphicsPipeline mGridPipeline;
    VulkanGraphicsPipeline mSsaoPipeline;
    VulkanGraphicsPipeline mSsaoBlurPipeline;
    VulkanGraphicsPipeline mTransparentCollectionPipeline;
    VulkanGraphicsPipeline mTransparencyResolutionPipeline;
    VulkanGraphicsPipeline mOpaqueForwardPassPipeline;
    VulkanGraphicsPipeline mForwardPassBlendPipeline;
    VulkanGraphicsPipeline mPostProcessingPipeline;
    VulkanGraphicsPipeline mLightIconPipeline;

    // forward+ rendering
    glm::uvec3 mClusterGridSize = glm::vec3(16, 16, 24);
    VulkanBuffer mVolumeClustersSSBO;
    VkPipelineLayout mFrustumClusterGenPipelineLayout{};
    VkPipeline mFrustumClusterGenPipeline{};
    VkPipelineLayout mAssignLightsToClustersPipelineLayout{};
    VkPipeline mAssignLightsToClustersPipeline{};

    // IBL
    VulkanTexture mIrradianceMap;
    VulkanTexture mPrefilterMap;
    VulkanTexture mBrdfLut;
    VulkanBuffer mIrradianceViewsUBO;
    VulkanBuffer mEnvMapViewsUBO;
    VkRenderPass mCubemapConvertRenderpass{};
    VkRenderPass mIrradianceConvolutionRenderpass{};
    VkRenderPass mPrefilterRenderpass{};
    VkRenderPass mBrdfLutRenderpass{};
    VulkanDsLayout mCubemapConvertDsLayout;
    VulkanDsLayout mIrradianceConvolutionDsLayout;
    VulkanGraphicsPipeline mCubemapConvertPipeline;
    VulkanGraphicsPipeline mIrradianceConvolutionPipeline;
    VulkanGraphicsPipeline mPrefilterPipeline;
    VulkanGraphicsPipeline mBrdfLutPipeline;
    VkFramebuffer mIrradianceConvolutionFramebuffer{};
    std::vector<VkFramebuffer> mPrefilterFramebuffers;
    VkFramebuffer mBrdfLutFramebuffer{};
    VulkanTexture mEquirectangularTexture;
    VulkanTexture mEnvMap;
    VkFramebuffer mCubemapConvertFramebuffer{};
    VkDescriptorSet mCubemapConvertDs{};
    VkDescriptorSet mIrradianceConvolutionDs{};
    uint32_t mEnvMapFaceSize{};
    float mSkyboxFov = 45.f;
    const uint32_t mMaxPrefilterMipLevels = 6;

    // Bloom
    static constexpr uint32_t BloomMipChainSize = 6;
    float mBrightnessThreshold = 1.f;
    float mFilterRadius = 0.005f;
    float mBloomStrength = 1.f;
    std::array<VulkanTexture, BloomMipChainSize> mBloomMipChain;
    VkRenderPass mCaptureBrightPixelsRenderpass{};
    VkFramebuffer mCaptureBrightPixelsFramebuffer{};
    VulkanGraphicsPipeline mCaptureBrightPixelsPipeline;
    VkRenderPass mBloomDownsampleRenderpass{};
    std::array<VkFramebuffer, BloomMipChainSize> mBloomDownsampleFramebuffers{};
    VulkanGraphicsPipeline mBloomDownsamplePipeline;
    VkRenderPass mBloomUpsampleRenderpass{};
    std::array<VkFramebuffer, BloomMipChainSize> mBloomUpsampleFramebuffers{};
    VulkanGraphicsPipeline mBloomUpsamplePipeline;
    std::array<VkDescriptorSet, BloomMipChainSize> mBloomMipChainDs{};

    // descriptor set layouts
    VulkanDsLayout mSingleImageDsLayout;
    VulkanDsLayout mCameraRenderDataDsLayout;
    VulkanDsLayout mMaterialsDsLayout;
    VulkanDsLayout mSSAODsLayout;
    VulkanDsLayout mOitResourcesDsLayout;
    VulkanDsLayout mIconTextureDsLayout;
    VulkanDsLayout mSingleInputAttachmentDsLayout;
    VulkanDsLayout mLightsDsLayout;
    VulkanDsLayout mFrustumClusterGenDsLayout;
    VulkanDsLayout mAssignLightsToClustersDsLayout;
    VulkanDsLayout mForwardShadingDsLayout;
    VulkanDsLayout mPostProcessingDsLayout;

    // descriptor sets
    VkDescriptorSet mCameraDs{};
    VkDescriptorSet mSsaoDs{};
    VkDescriptorSet mSsaoTextureDs{};
    VkDescriptorSet mSsaoBlurTextureDs{};
    VkDescriptorSet mDepthDs{};
    VkDescriptorSet mSkyboxDs{};
    VkDescriptorSet mColor32FDs{};
    VkDescriptorSet mColor8UDs{};
    VkDescriptorSet mColor32FInputDs{};
    VkDescriptorSet mLightsDs{};
    VkDescriptorSet mFrustumClusterGenDs{};
    VkDescriptorSet mAssignLightsToClustersDs{};
    VkDescriptorSet mForwardShadingDs{};
    VkDescriptorSet mPostProcessingDs{};

    // ssao
    std::uniform_real_distribution<float> mSsaoDistribution {0.f, 1.f};
    std::default_random_engine mSsaoRandomEngine;
    float mSsaoRadius = 0.5f;
    float mSsaoIntensity = 1.f;
    float mSsaoDepthBias = 0.025f;
    std::vector<glm::vec4> mSsaoKernel;
    VulkanBuffer mSsaoKernelSSBO;
    VulkanTexture mSsaoNoiseTexture;

    // gizmo icons
    VulkanTexture mTranslateIcon;
    VulkanTexture mRotateIcon;
    VulkanTexture mScaleIcon;
    VulkanTexture mGlobalIcon;
    VulkanTexture mLocalIcon;

    VkDescriptorSet mTranslateIconDs{};
    VkDescriptorSet mRotateIconDs{};
    VkDescriptorSet mScaleIconDs{};
    VkDescriptorSet mGlobalIconDs{};
    VkDescriptorSet mLocalIconDs{};

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

    bool mHDROn = false;
    Tonemap mTonemap;
    float mExposure = 1.f;
    float mMaxWhite = 4.f;

    struct GridData
    {
        alignas(16) glm::vec4 thinLineColor = glm::vec4(0.5f, 0.5f, 0.5f, 80.f / 255.f);
        alignas(16) glm::vec4 thickLineColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.f);
    } mGridData;

    bool mRenderSkybox = true;
    bool mEnableIblLighting = true;
    bool mSsaoOn = true;
    bool mOitOn = true;
    bool mBloomOn = true;
    bool mRenderGrid = true;
    bool mDebugNormals = false;

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

struct Cluster
{
    alignas(16) glm::vec4 minPoint;
    alignas(16) glm::vec4 maxPoint;
    alignas(4) uint32_t lightCount;
    alignas(16) uint32_t lightIndices[PerClusterCapacity];
};

enum class Tonemap
{
    Reinhard = 1,
    ReinhardExtended = 2,
    NarkowiczAces = 3,
    HillAces = 4
};

inline const char* toStr(Tonemap t)
{
    switch (t)
    {
        case Tonemap::Reinhard: return "Reinhard";
        case Tonemap::ReinhardExtended: return "ReinhardExtended";
        case Tonemap::NarkowiczAces: return "NarkowiczAces";
        case Tonemap::HillAces: return "HillAces";
        default: return "Unknown";
    }
}

#include "renderer.inl"

#endif //VULKANRENDERINGENGINE_RENDERER_HPP
