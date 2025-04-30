//
// Created by Gianni on 4/01/2025.
//

#ifndef VULKANRENDERINGENGINE_RENDERER_HPP
#define VULKANRENDERINGENGINE_RENDERER_HPP

#include "../utils/utils.hpp"
#include "../app/save_data.hpp"
#include "../vk/vulkan_pipeline.hpp"
#include "../scene_graph/scene_graph.hpp"
#include "camera.hpp"
#include "model.hpp"
#include "model_importer.hpp"
#include "lights.hpp"

constexpr uint32_t InitialViewportWidth = 1000;
constexpr uint32_t InitialViewportHeight = 700;
constexpr uint32_t MaxDirLights = 128;
constexpr uint32_t MaxPointLights = 128;
constexpr uint32_t MaxSpotLights = 128;
constexpr uint32_t MaxShadowMapsPerType = 50;
constexpr uint32_t MaxSsaoKernelSamples = 128;
constexpr uint32_t SsaoNoiseTextureSize = 4;
constexpr uint32_t PerClusterCapacity = 32;
constexpr VkSampleCountFlagBits SampleCount = VK_SAMPLE_COUNT_8_BIT;

struct TransparentMesh;
struct LightIconRenderData;
struct Cluster;
enum class Tonemap;

class Renderer
{
public:
    Renderer(const VulkanRenderDevice& renderDevice, SaveData& saveData);
    ~Renderer();

    void update();
    void render(VkCommandBuffer commandBuffer);

    void importModel(const ModelImportData& importData);
    void importEnvMap(const std::string& path);
    void resize(uint32_t width, uint32_t height);

    void addDirLight(uuid32_t id, const DirectionalLight& light);
    void addPointLight(uuid32_t id, const PointLight& light);
    void addSpotLight(uuid32_t id, const SpotLight& light);
    DirectionalLight& getDirLight(uuid32_t id);
    PointLight& getPointLight(uuid32_t id);
    SpotLight& getSpotLight(uuid32_t id);
    void updateDirLight(uuid32_t id);
    void updatePointLight(uuid32_t id);
    void updateSpotLight(uuid32_t id);
    void deleteDirLight(uuid32_t id);
    void deletePointLight(uuid32_t id);
    void deleteSpotLight(uuid32_t id);

private:
    void executeDirShadowRenderpass(VkCommandBuffer commandBuffer);
    void executePointShadowRenderpass(VkCommandBuffer commandBuffer);
    void executeSpotShadowRenderpass(VkCommandBuffer commandBuffer);
    void executePrepass(VkCommandBuffer commandBuffer);
    void executeSkyboxRenderpass(VkCommandBuffer commandBuffer);
    void executeSsaoResourcesRenderpass(VkCommandBuffer commandBuffer);
    void executeSsaoRenderpass(VkCommandBuffer commandBuffer);
    void executeSsaoBlurRenderpass(VkCommandBuffer commandBuffer);
    void executeGenFrustumClustersRenderpass(VkCommandBuffer commandBuffer);
    void executeAssignLightsToClustersRenderpass(VkCommandBuffer commandBuffer);
    void executeForwardRenderpass(VkCommandBuffer commandBuffer);
    void executeBloomRenderpass(VkCommandBuffer commandBuffer);
    void executePostProcessingRenderpass(VkCommandBuffer commandBuffer);
    void executeWireframeRenderpass(VkCommandBuffer commandBuffer);
    void executeGridRenderpass(VkCommandBuffer commandBuffer);
    void executeLightIconRenderpass(VkCommandBuffer commandBuffer);
    void setViewport(VkCommandBuffer commandBuffer);
    void updateCameraUBO();
    void getLightIconRenderData();
    void sortTransparentMeshes();
    void bindTexture(VkCommandBuffer commandBuffer,
                     VkPipelineLayout pipelineLayout,
                     const VulkanTexture& texture,
                     uint32_t binding,
                     uint32_t set);

    void updateImports();
    void addModel(ModelLoader& modelData);
    void addMeshes(Model& model, ModelLoader& modelData);
    void addTextures(Model& model, ModelLoader& modelData);

    void updateSceneGraph();
    void updateGraphNode(NodeType type, GraphNode* node);
    void updateEmptyNode(GraphNode* node);
    void updateMeshNode(GraphNode* node);
    void updateDirLightNode(GraphNode* node);
    void updatePointLightNode(GraphNode* node);
    void updateSpotLightNode(GraphNode* node);

    void createColorTexture32MS();
    void createColorTexture8U();
    void createDepthTextures();
    void createViewPosTexture();
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

    void addDirShadowMap(); // Dir shadows
    void updateDirShadowsMaps();
    void updateDirShadowMapImage(uuid32_t id);
    void createDirShadowMap(DirShadowMap& shadowMap, index_t index);
    void deleteDirShadowMap(uuid32_t id);
    DirShadowData& getDirShadowData(uuid32_t id);
    DirShadowMap& getDirShadowMap(uuid32_t id);
    void addPointShadowMap(); // point shadows
    void updatePointShadowMapData(uuid32_t id);
    void updatePointShadowMapImage(uuid32_t id);
    void createPointShadowMap(PointShadowMap& shadowMap, index_t index, uint32_t resolution);
    void deletePointShadowMap(uuid32_t id);
    PointShadowData& getPointShadowData(uuid32_t id);
    PointShadowMap& getPointShadowMap(uuid32_t id);
    void addSpotShadowMap(); // spot shadows
    void updateSpotShadowMapData(uuid32_t id);
    void updateSpotShadowMapImage(uuid32_t id);
    void createSpotShadowMap(SpotShadowMap& shadowMap, index_t index, uint32_t resolution);
    void deleteSpotShadowMap(uuid32_t id);
    SpotShadowData& getSpotShadowData(uuid32_t id);
    SpotShadowMap& getSpotShadowMap(uuid32_t id);
    void createShadowMapBuffers();
    void createShadowMapSamplers();
    void createDirShadowRenderpass();
    void createDirShadowPipeline();
    void createSpotShadowRenderpass();
    void createSpotShadowPipeline();
    void createPointShadowRenderpass();
    void createPointShadowPipeline();

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
    void createSsaoResourcesRenderpass();
    void createSsaoResourcesFramebuffer();
    void createSsaoResourcesPipeline();
    void createSsaoRenderpass();
    void createSsaoFramebuffer();
    void createSsaoBlurRenderpass();
    void createSsaoBlurFramebuffers();
    void createSsaoPipeline();
    void createSsaoBlurPipeline();

    void createForwardRenderpass();
    void createForwardFramebuffer();
    void createOpaqueForwardPassPipeline();
    void createTransparentForwardPassPipeline();

    void createPostProcessingRenderpass();
    void createPostProcessingFramebuffer();
    void createPostProcessingPipeline();

    void createWireframeRenderpass();
    void createWireframeFramebuffer();
    void createWireframePipeline();

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

    // render targets
    VulkanTexture mColorTexture32MS;
    VulkanTexture mColorResolve32;
    VulkanTexture mDepthTextureMS;
    VulkanTexture mColorTexture8U;
    VulkanTexture mDepthTexture;
    VulkanTexture mViewPosTexture;
    VulkanTexture mNormalTexture;
    VulkanTexture mSsaoTexture;
    VulkanTexture mSsaoBlurTexture1;
    VulkanTexture mSsaoBlurTexture2;

    // render passes
    VkRenderPass mPrepassRenderpass{};
    VkRenderPass mSkyboxRenderpass{};
    VkRenderPass mSsaoResourcesRenderpass{};
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
    VkFramebuffer mSsaoResourcesFramebuffer{};
    VkFramebuffer mSsaoFramebuffer{};
    VkFramebuffer mSsaoBlurFramebuffer1{};
    VkFramebuffer mSsaoBlurFramebuffer2{};
    VkFramebuffer mGridFramebuffer{};
    VkFramebuffer mPostProcessingFramebuffer{};
    VkFramebuffer mLightIconFramebuffer{};

    // graphics pipelines
    VulkanGraphicsPipeline mPrepassPipeline;
    VulkanGraphicsPipeline mSkyboxPipeline;
    VulkanGraphicsPipeline mGridPipeline;
    VulkanGraphicsPipeline mSsaoResourcesPipeline;
    VulkanGraphicsPipeline mSsaoPipeline;
    VulkanGraphicsPipeline mSsaoBlurPipeline;
    VulkanGraphicsPipeline mOpaqueForwardPassPipeline;
    VulkanGraphicsPipeline mTransparentForwardPassPipeline;
    VulkanGraphicsPipeline mPostProcessingPipeline;
    VulkanGraphicsPipeline mLightIconPipeline;

    // Shadow mapping
    VkRenderPass mDirShadowRenderpass{};
    VkRenderPass mPointShadowRenderpass{};
    VkRenderPass mSpotShadowRenderpass{};
    VulkanGraphicsPipeline mDirShadowPipeline;
    VulkanGraphicsPipeline mPointShadowPipeline;
    VulkanGraphicsPipeline mSpotShadowPipeline;
    VulkanSampler mDirShadowMapSampler{};
    VulkanSampler mPointShadowMapSampler{};
    VulkanSampler mSpotShadowMapSampler{};

    // ssao
    std::uniform_real_distribution<float> mSsaoDistribution {0.f, 1.f};
    std::default_random_engine mSsaoRandomEngine;
    float mSsaoRadius = 0.5f;
    float mSsaoIntensity = 1.f;
    float mSsaoDepthBias = 0.025f;
    std::vector<glm::vec4> mSsaoKernel;
    VulkanBuffer mSsaoKernelSSBO;
    VulkanTexture mSsaoNoiseTexture;

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

    // Wireframe
    glm::vec4 mWireframeColor {0.f, 1.f, 0.f, 1.f};
    float mWireframeWidth = 2.f;
    VkRenderPass mWireframeRenderpass{};
    VkFramebuffer mWireframeFramebuffer{};
    VulkanGraphicsPipeline mWireframePipeline;

    // Bloom
    static constexpr uint32_t BloomMipChainSize = 6;
    float mThreshold = 1.2f;
    float mFilterRadius = 1.f;
    float mBloomStrength = 1.f;
    float mKnee = 1.f;
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
    VkDescriptorSet mSsaoBlurTexture1Ds{};
    VkDescriptorSet mSsaoBlurTexture2Ds{};
    VkDescriptorSet mDepthDs{};
    VkDescriptorSet mSkyboxDs{};
    VkDescriptorSet mColorResolve32Ds{};
    VkDescriptorSet mColor8UDs{};
    VkDescriptorSet mLightsDs{};
    VkDescriptorSet mFrustumClusterGenDs{};
    VkDescriptorSet mAssignLightsToClustersDs{};
    VkDescriptorSet mForwardShadingDs{};
    VkDescriptorSet mPostProcessingDs{};

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
    std::vector<std::future<ModelLoader>> mModelDataFutures;
    std::unordered_map<uuid32_t, Model> mModels;
    std::multiset<TransparentMesh> mSortedTransparentMeshes;

    // lights
    std::vector<DirectionalLight> mDirLights;
    std::vector<PointLight> mPointLights;
    std::vector<SpotLight> mSpotLights;

    std::unordered_map<uuid32_t, index_t> mUuidToDirLightIndex;
    std::unordered_map<uuid32_t, index_t> mUuidToPointLightIndex;
    std::unordered_map<uuid32_t, index_t> mUuidToSpotLightIndex;

    VulkanBuffer mDirLightSSBO;
    VulkanBuffer mPointLightSSBO;
    VulkanBuffer mSpotLightSSBO;

    // Shadow data
    std::vector<DirShadowData> mDirShadowData;
    std::vector<PointShadowData> mPointShadowData;
    std::vector<SpotShadowData> mSpotShadowData;

    std::vector<DirShadowMap> mDirShadowMaps;
    std::vector<PointShadowMap> mPointShadowMaps;
    std::vector<SpotShadowMap> mSpotShadowMaps;

    VulkanBuffer mDirShadowDataSSBO;
    VulkanBuffer mPointShadowDataSSBO;
    VulkanBuffer mSpotShadowDataSSBO;

    // Light icons
    VulkanTexture mDirLightIcon;
    VulkanTexture mPointLightIcon;
    VulkanTexture mSpotLightIcon;

    // render data
    glm::vec4 mClearColor = glm::vec4(0.251f, 0.235f, 0.235f, 1.f);
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

    bool mRenderSkybox = false;
    bool mEnableIblLighting = false;
    bool mSsaoOn = false;
    bool mBloomOn = false;
    bool mRenderGrid = true;
    bool mDebugNormals = false;
    bool mWireframeOn = false;

private:
    friend class Editor;
};

struct TransparentMesh
{
    float distance;
    uuid32_t modelID;
    Mesh* mesh;
    glm::mat4 model;

    bool operator<(const TransparentMesh& other) const
    {
        return distance < other.distance;
    }
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
