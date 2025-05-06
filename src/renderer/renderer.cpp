//
// Created by Gianni on 4/01/2025.
//

#include "renderer.hpp"

namespace ImGuizmo
{
    void DecomposeMatrixToComponents(const float* matrix, float* translation, float* rotation, float* scale);
}

Renderer::Renderer(const VulkanRenderDevice& renderDevice, SaveData& saveData)
    : mRenderDevice(renderDevice)
    , mSaveData(saveData)
    , mSceneGraph(std::make_shared<SceneGraph>())
    , mWidth(InitialViewportWidth)
    , mHeight(InitialViewportHeight)
    , mCameraUBO(renderDevice, sizeof(CameraRenderData), BufferType::Uniform, MemoryType::HostCoherent)
    , mTonemap(Tonemap::ReinhardExtended)
{
    if (saveData.contains("viewport"))
    {
        mWidth = saveData["viewport"]["width"];
        mHeight = saveData["viewport"]["height"];
    }

    mCamera = Camera(glm::vec3(0.f, 1.f, 0.f),
                     40.f,
                     static_cast<float>(mWidth),
                     static_cast<float>(mHeight));

    createDefaultMaterialTextures(mRenderDevice);

    createColorTexture32MS();
    createDepthTextures();
    createViewPosTexture();
    createNormalTexture();
    createSsaoTextures();
    createColorTexture8U();

    createSingleImageDsLayout();
    createCameraRenderDataDsLayout();
    createMaterialsDsLayout();
    createSsaoDsLayout();
    createOitResourcesDsLayout();
    createSingleInputAttachmentDsLayout();
    createLightsDsLayout();
    createFrustumClusterGenDsLayout();
    createAssignLightsToClustersDsLayout();
    createForwardShadingDsLayout();
    createPostProcessingDsLayout();

    createShadowMapBuffers();
    createShadowMapSamplers();
    createSpotShadowRenderpass();
    createSpotShadowPipeline();
    createDirShadowRenderpass();
    createDirShadowPipeline();
    createPointShadowRenderpass();
    createPointShadowPipeline();

    createPrepassRenderpass();
    createPrepassFramebuffer();
    createPrepassPipeline();

    createVolumeClusterSSBO();
    createFrustumClusterGenPipelineLayout();
    createFrustumClusterGenPipeline();
    createAssignLightsToClustersPipelineLayout();
    createAssignLightsToClustersPipeline();

    createSkyboxRenderpass();
    createSkyboxFramebuffer();
    createSkyboxPipeline();

    createSsaoKernel(32);
    createSsaoKernelSSBO();
    updateSsaoKernelSSBO();
    createSsaoNoiseTexture();
    createSsaoResourcesRenderpass();
    createSsaoResourcesFramebuffer();
    createSsaoResourcesPipeline();
    createSsaoRenderpass();
    createSsaoFramebuffer();
    createSsaoBlurRenderpass();
    createSsaoBlurFramebuffers();
    createSsaoPipeline();
    createSsaoBlurPipeline();

    createForwardRenderpass();
    createForwardFramebuffer();
    createOpaqueForwardPassPipeline();
    createTransparentForwardPassPipeline();

    createGridRenderpass();
    createGridFramebuffer();
    createGridPipeline();

    createPostProcessingRenderpass();
    createPostProcessingFramebuffer();
    createPostProcessingPipeline();

    createWireframeRenderpass();
    createWireframeFramebuffer();
    createWireframePipeline();

    createLightBuffers();
    createLightIconTextures();
    createLightIconTextureDsLayout();
    createLightIconRenderpass();
    createLightIconFramebuffer();
    createLightIconPipeline();

    createShadowMapBuffers();

    createGizmoIconResources();

    createIrradianceMap();
    createPrefilterMap();
    createBrdfLut();
    createEnvMapViewsUBO();
    createIrradianceViewsUBO();
    createCubemapConvertRenderpass();
    createIrradianceConvolutionRenderpass();
    createPrefilterRenderpass();
    createBrdfLutRenderpass();
    createCubemapConvertDsLayout();
    createIrradianceConvolutionDsLayout();
    createCubemapConvertPipeline();
    createConvolutionPipeline();
    createPrefilterPipeline();
    createBrdfLutPipeline();
    createIrradianceConvolutionFramebuffer();
    createPrefilterFramebuffers();
    createBrdfLutFramebuffer();
    importEnvMap("assets/cubemaps/puresky.hdr");

    createBloomMipChain();
    createCaptureBrightPixelsRenderpass();
    createCaptureBrightPixelsFramebuffer();
    createCaptureBrightPixelsPipeline();
    createBloomDownsampleRenderpass();
    createBloomDownsampleFramebuffers();
    createBloomDownsamplePipeline();
    createBloomUpsampleRenderpass();
    createBloomUpsampleFramebuffers();
    createBloomUpsamplePipeline();
    createBloomMipChainDs();

    createCameraDs();
    createSingleImageDescriptorSets();
    createSsaoDs();
    createLightsDs();
    createFrustumClusterGenDs();
    createAssignLightsToClustersDs();
    createForwardShadingDs();
    updateForwardShadingDs();
    createPostProcessingDs();

    importModels();
}

Renderer::~Renderer()
{
    destroyDefaultMaterialTextures();

    vkDestroySampler(mRenderDevice.device, mDirShadowMapSampler.sampler, nullptr);
    vkDestroySampler(mRenderDevice.device, mPointShadowMapSampler.sampler, nullptr);
    vkDestroySampler(mRenderDevice.device, mSpotShadowMapSampler.sampler, nullptr);

    vkDestroyPipeline(mRenderDevice.device, mFrustumClusterGenPipeline, nullptr);
    vkDestroyPipeline(mRenderDevice.device, mAssignLightsToClustersPipeline, nullptr);

    vkDestroyPipelineLayout(mRenderDevice.device, mFrustumClusterGenPipelineLayout, nullptr);
    vkDestroyPipelineLayout(mRenderDevice.device, mAssignLightsToClustersPipelineLayout, nullptr);

    vkDestroyFramebuffer(mRenderDevice.device, mPrepassFramebuffer, nullptr);
    vkDestroyFramebuffer(mRenderDevice.device, mSkyboxFramebuffer, nullptr);
    vkDestroyFramebuffer(mRenderDevice.device, mForwardPassFramebuffer, nullptr);
    vkDestroyFramebuffer(mRenderDevice.device, mSsaoFramebuffer, nullptr);
    vkDestroyFramebuffer(mRenderDevice.device, mSsaoBlurFramebuffer1, nullptr);
    vkDestroyFramebuffer(mRenderDevice.device, mSsaoBlurFramebuffer2, nullptr);
    vkDestroyFramebuffer(mRenderDevice.device, mGridFramebuffer, nullptr);
    vkDestroyFramebuffer(mRenderDevice.device, mPostProcessingFramebuffer, nullptr);
    vkDestroyFramebuffer(mRenderDevice.device, mLightIconFramebuffer, nullptr);
    vkDestroyFramebuffer(mRenderDevice.device, mCubemapConvertFramebuffer, nullptr);
    vkDestroyFramebuffer(mRenderDevice.device, mIrradianceConvolutionFramebuffer, nullptr);
    vkDestroyFramebuffer(mRenderDevice.device, mBrdfLutFramebuffer, nullptr);
    vkDestroyFramebuffer(mRenderDevice.device, mCaptureBrightPixelsFramebuffer, nullptr);
    vkDestroyFramebuffer(mRenderDevice.device, mWireframeFramebuffer, nullptr);
    vkDestroyFramebuffer(mRenderDevice.device, mSsaoResourcesFramebuffer, nullptr);
    for (auto fb : mPrefilterFramebuffers)
        vkDestroyFramebuffer(mRenderDevice.device, fb, nullptr);
    for (auto fb : mBloomDownsampleFramebuffers)
        vkDestroyFramebuffer(mRenderDevice.device, fb, nullptr);
    for (auto fb : mBloomUpsampleFramebuffers)
        vkDestroyFramebuffer(mRenderDevice.device, fb, nullptr);
    for (auto& shadowMap : mDirShadowMaps)
        for (auto& fb : shadowMap.framebuffers)
            vkDestroyFramebuffer(mRenderDevice.device, fb, nullptr);
    for (auto& shadowMap : mPointShadowMaps)
        for (auto & framebuffer : shadowMap.framebuffers)
            vkDestroyFramebuffer(mRenderDevice.device, framebuffer, nullptr);
    for (auto& shadowMap : mSpotShadowMaps)
        vkDestroyFramebuffer(mRenderDevice.device, shadowMap.framebuffer, nullptr);

    vkDestroyRenderPass(mRenderDevice.device, mPrepassRenderpass, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mSkyboxRenderpass, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mSsaoRenderpass, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mSsaoBlurRenderpass, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mForwardRenderpass, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mGridRenderpass, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mPostProcessingRenderpass, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mLightIconRenderpass, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mCubemapConvertRenderpass, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mIrradianceConvolutionRenderpass, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mPrefilterRenderpass, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mBrdfLutRenderpass, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mCaptureBrightPixelsRenderpass, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mBloomDownsampleRenderpass, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mBloomUpsampleRenderpass, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mDirShadowRenderpass, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mPointShadowRenderpass, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mSpotShadowRenderpass, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mWireframeRenderpass, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mSsaoResourcesRenderpass, nullptr);
}

void Renderer::update()
{
    updateImports();
    updateCameraUBO();
    updateSceneGraph();
    updateDirShadowsMaps();
    sortTransparentMeshes();
}

void Renderer::render(VkCommandBuffer commandBuffer)
{
    executeDirShadowRenderpass(commandBuffer);
    executePointShadowRenderpass(commandBuffer);
    executeSpotShadowRenderpass(commandBuffer);
    setViewport(commandBuffer);
    executePrepass(commandBuffer);
    executeSkyboxRenderpass(commandBuffer);
    executeSsaoResourcesRenderpass(commandBuffer);
    executeSsaoRenderpass(commandBuffer);
    executeSsaoBlurRenderpass(commandBuffer);
    executeGenFrustumClustersRenderpass(commandBuffer);
    executeAssignLightsToClustersRenderpass(commandBuffer);
    executeForwardRenderpass(commandBuffer);
    executeBloomRenderpass(commandBuffer);
    executePostProcessingRenderpass(commandBuffer);
    executeWireframeRenderpass(commandBuffer);
    executeGridRenderpass(commandBuffer);
    executeLightIconRenderpass(commandBuffer);
}

void Renderer::importModel(const ModelImportData& importData)
{
    bool loaded = std::find_if(mModels.begin(), mModels.end(), [&importData] (const auto& pair) {
        return importData.path == pair.second.path;
    }) != mModels.end();

    if (loaded)
    {
        debugLog("Model is already loaded.");
        return;
    }

    auto future = std::async(std::launch::async, [importData] () {
        return ModelLoader(importData);
    });

    mModelDataFutures.push_back(std::move(future));
}

void Renderer::importEnvMap(const std::string& path)
{
    createEquirectangularTexture(path);
    createEnvMap();
    createSkyboxDs();
    createCubemapConvertFramebuffer();
    createCubemapConvertDs();
    createIrradianceConvolutionDs();

    executeCubemapConvertRenderpass();
    executeIrradianceConvolutionRenderpass();
    executePrefilterRenderpasses();
    executeBrdfLutRenderpass();
}

void Renderer::resize(uint32_t width, uint32_t height)
{
    mWidth = width;
    mHeight = height;

    mCamera.resize(width, height);

    std::vector<VkDescriptorSet> freeDs {
        mSsaoDs,
        mSsaoTextureDs,
        mSsaoBlurTexture1Ds,
        mSsaoBlurTexture2Ds,
        mDepthDs,
        mColorResolve32Ds,
        mColor8UDs
    };

    vkFreeDescriptorSets(mRenderDevice.device, mRenderDevice.descriptorPool, freeDs.size(), freeDs.data());

    createColorTexture32MS();
    createDepthTextures();
    createViewPosTexture();
    createNormalTexture();
    createSsaoTextures();
    createColorTexture8U();
    createBloomMipChain();

    createPrepassFramebuffer();
    createSkyboxFramebuffer();
    createSsaoFramebuffer();
    createSsaoBlurFramebuffers();
    createForwardFramebuffer();
    createGridFramebuffer();
    createPostProcessingFramebuffer();
    createLightIconFramebuffer();
    createCaptureBrightPixelsFramebuffer();
    createBloomDownsampleFramebuffers();
    createBloomUpsampleFramebuffers();
    createWireframeFramebuffer();
    createSsaoResourcesFramebuffer();

    createSingleImageDescriptorSets();
    createSsaoDs();
    updateForwardShadingDs();
    createBloomMipChainDs();
    createPostProcessingDs();
}

void Renderer::addDirLight(uuid32_t id, const DirectionalLight& light, const DirShadowData& shadowData)
{
    addLight(mUuidToDirLightIndex, mDirLights, mDirLightSSBO, id, light);
    addDirShadowMap(shadowData);
}

void Renderer::addPointLight(uuid32_t id, const PointLight& light, const PointShadowData& shadowData)
{
    addLight(mUuidToPointLightIndex, mPointLights, mPointLightSSBO, id, light);
    addPointShadowMap(shadowData);
}

void Renderer::addSpotLight(uuid32_t id, const SpotLight& light, const SpotShadowData& shadowData)
{
    addLight(mUuidToSpotLightIndex, mSpotLights, mSpotLightSSBO, id, light);
    addSpotShadowMap(shadowData);
}

DirectionalLight &Renderer::getDirLight(uuid32_t id)
{
    return getLight(mUuidToDirLightIndex, mDirLights, id);
}

PointLight &Renderer::getPointLight(uuid32_t id)
{
    return getLight(mUuidToPointLightIndex, mPointLights, id);
}

SpotLight &Renderer::getSpotLight(uuid32_t id)
{
    return getLight(mUuidToSpotLightIndex, mSpotLights, id);
}

void Renderer::updateDirLight(uuid32_t id)
{
    updateLight(mUuidToDirLightIndex, mDirLights, mDirLightSSBO, id);
}

void Renderer::updatePointLight(uuid32_t id)
{
    updateLight(mUuidToPointLightIndex, mPointLights, mPointLightSSBO, id);

    // update shadow map
    index_t i = mUuidToPointLightIndex.at(id);
    calcMatrices(mPointShadowData.at(i), mPointLights.at(i));
    mPointShadowDataSSBO.update(i * sizeof(PointShadowData), sizeof(PointShadowData), &mPointShadowData.at(i));
}

void Renderer::updateSpotLight(uuid32_t id)
{
    updateLight(mUuidToSpotLightIndex, mSpotLights, mSpotLightSSBO, id);

    // update shadow map
    index_t i = mUuidToSpotLightIndex.at(id);
    calcMatrices(mSpotShadowData.at(i), mSpotLights.at(i));
    mSpotShadowDataSSBO.update(i * sizeof(SpotShadowData), sizeof(SpotShadowData), &mSpotShadowData.at(i));
}

void Renderer::deleteDirLight(uuid32_t id)
{
    deleteDirShadowMap(id);
    deleteLight(mUuidToDirLightIndex, mDirLights, mDirLightSSBO, id);
}

void Renderer::deletePointLight(uuid32_t id)
{
    deletePointShadowMap(id);
    deleteLight(mUuidToPointLightIndex, mPointLights, mPointLightSSBO, id);
}

void Renderer::deleteSpotLight(uuid32_t id)
{
    deleteSpotShadowMap(id);
    deleteLight(mUuidToSpotLightIndex, mSpotLights, mSpotLightSSBO, id);
}

void Renderer::executeDirShadowRenderpass(VkCommandBuffer commandBuffer)
{
    beginDebugLabel(commandBuffer, "Gen Dir Shadows Maps");

    constexpr VkClearValue depthClear {.depthStencil = {.depth = 1.f, .stencil = 0}};
    for (uint32_t i = 0; i < mDirLights.size(); ++i)
    {
        const DirShadowData& dsd = mDirShadowData.at(i);
        if (dsd.shadowType == ShadowType::NoShadow)
            continue;

        uint32_t resolution = dsd.resolution;
        VkRenderPassBeginInfo renderPassBeginInfo {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = mDirShadowRenderpass,
            .framebuffer = VK_NULL_HANDLE,
            .renderArea = {
                .offset = {.x = 0, .y = 0},
                .extent = {
                    .width = resolution,
                    .height = resolution
                }
            },
            .clearValueCount = 1,
            .pClearValues = &depthClear
        };

        VkViewport viewport {
            .x = 0.f,
            .y = 0.f,
            .width = static_cast<float>(resolution),
            .height = static_cast<float>(resolution),
            .minDepth = 0.f,
            .maxDepth = 1.f
        };

        VkRect2D scissor {
            .offset = {.x = 0, .y = 0},
            .extent = {
                .width = resolution,
                .height = resolution
            }
        };

        for (uint32_t ii = 0; ii < dsd.cascadeCount; ++ii)
        {
            renderPassBeginInfo.framebuffer = mDirShadowMaps.at(i).framebuffers.at(ii);

            std::string debugLabel = std::format("Shadow map {}, cascade {}", i, ii);
            insertDebugLabel(commandBuffer, debugLabel.data());
            vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mDirShadowPipeline);

            vkCmdPushConstants(commandBuffer,
                               mDirShadowPipeline,
                               VK_SHADER_STAGE_VERTEX_BIT,
                               0, sizeof(glm::mat4),
                               glm::value_ptr(dsd.viewProj[ii]));

            for (const auto& [id, model] : mModels)
            {
                // cull front face to prevent peter panning
                pfnCmdSetCullModeEXT(commandBuffer, VK_CULL_MODE_FRONT_BIT);
                pfnCmdSetFrontFaceEXT(commandBuffer, model.frontFace);

                for (const auto& mesh : model.meshes)
                    if (model.drawOpaque(mesh))
                        mesh.mesh.render(commandBuffer);
            }

            vkCmdEndRenderPass(commandBuffer);
        }
    }

    endDebugLabel(commandBuffer);
}

void Renderer::executePointShadowRenderpass(VkCommandBuffer commandBuffer)
{
    beginDebugLabel(commandBuffer, "Gen Point Shadow Maps");

    constexpr VkClearValue depthClear {.depthStencil = {.depth = 1.f, .stencil = 0}};
    for (uint32_t i = 0; i < mPointLights.size(); ++i)
    {
        const PointShadowData& psd = mPointShadowData.at(i);
        if (psd.shadowType == ShadowType::NoShadow)
            continue;

        uint32_t resolution = psd.resolution;
        VkRenderPassBeginInfo renderPassBeginInfo {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = mPointShadowRenderpass,
            .framebuffer = VK_NULL_HANDLE,
            .renderArea = {
                .offset = {.x = 0, .y = 0},
                .extent = {
                    .width = resolution,
                    .height = resolution
                }
            },
            .clearValueCount = 1,
            .pClearValues = &depthClear
        };

        VkViewport viewport {
            .x = 0.f,
            .y = 0.f,
            .width = static_cast<float>(resolution),
            .height = static_cast<float>(resolution),
            .minDepth = 0.f,
            .maxDepth = 1.f
        };

        VkRect2D scissor {
            .offset = {.x = 0, .y = 0},
            .extent = {
                .width = resolution,
                .height = resolution
            }
        };

        for (uint32_t ii = 0; ii < 6; ++ii)
        {
            renderPassBeginInfo.framebuffer = mPointShadowMaps.at(i).framebuffers[ii];

            std::string debugLabel = std::format("Shadow map {}, face {}", i, ii);
            insertDebugLabel(commandBuffer, debugLabel.data());
            vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPointShadowPipeline);

            vkCmdPushConstants(commandBuffer,
                               mPointShadowPipeline,
                               VK_SHADER_STAGE_VERTEX_BIT,
                               0, sizeof(glm::mat4),
                               glm::value_ptr(psd.viewProj[ii]));

            struct {
                glm::vec4 lightPos;
                float nearPlane;
                float farPlane;
            } fragPushConst {
                mPointLights.at(i).position,
                0.1f,
                mPointLights.at(i).range
            };

            vkCmdPushConstants(commandBuffer,
                               mPointShadowPipeline,
                               VK_SHADER_STAGE_FRAGMENT_BIT,
                               sizeof(glm::mat4), sizeof(fragPushConst),
                               &fragPushConst);

            for (const auto& [id, model] : mModels)
            {
                // cull front face to prevent peter panning
                pfnCmdSetCullModeEXT(commandBuffer, VK_CULL_MODE_FRONT_BIT);
                pfnCmdSetFrontFaceEXT(commandBuffer, model.frontFace);

                for (const auto& mesh : model.meshes)
                    if (model.drawOpaque(mesh))
                        mesh.mesh.render(commandBuffer);
            }

            vkCmdEndRenderPass(commandBuffer);
        }
    }

    endDebugLabel(commandBuffer);
}

void Renderer::executeSpotShadowRenderpass(VkCommandBuffer commandBuffer)
{
    beginDebugLabel(commandBuffer, "Generate Spot Shadow Maps");

    constexpr VkClearValue depthClear {.depthStencil = {.depth = 1.f, .stencil = 0}};
    for (uint32_t i = 0; i < mSpotLights.size(); ++i)
    {
        if (mSpotShadowData.at(i).shadowType == ShadowType::NoShadow)
            continue;

        uint32_t resolution = mSpotShadowData.at(i).resolution;

        VkRenderPassBeginInfo renderPassBeginInfo {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = mSpotShadowRenderpass,
            .framebuffer = mSpotShadowMaps.at(i).framebuffer,
            .renderArea = {
                .offset = {.x = 0, .y = 0},
                .extent = {
                    .width = resolution,
                    .height = resolution
                }
            },
            .clearValueCount = 1,
            .pClearValues = &depthClear
        };

        VkViewport viewport {
            .x = 0.f,
            .y = 0.f,
            .width = static_cast<float>(resolution),
            .height = static_cast<float>(resolution),
            .minDepth = 0.f,
            .maxDepth = 1.f
        };

        VkRect2D scissor {
            .offset = {.x = 0, .y = 0},
            .extent = {
                .width = resolution,
                .height = resolution
            }
        };
        
        vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mSpotShadowPipeline);

        vkCmdPushConstants(commandBuffer,
                   mSpotShadowPipeline,
                   VK_SHADER_STAGE_VERTEX_BIT,
                   0, sizeof(glm::mat4),
                   glm::value_ptr(mSpotShadowData.at(i).viewProj));

        for (const auto& [id, model] : mModels)
        {
            // cull front face to prevent peter panning
            pfnCmdSetCullModeEXT(commandBuffer, VK_CULL_MODE_FRONT_BIT);
            pfnCmdSetFrontFaceEXT(commandBuffer, model.frontFace);

            for (const auto& mesh : model.meshes)
                if (model.drawOpaque(mesh))
                    mesh.mesh.render(commandBuffer);
        }

        vkCmdEndRenderPass(commandBuffer);
    }

    endDebugLabel(commandBuffer);
}

void Renderer::executePrepass(VkCommandBuffer commandBuffer)
{
    VkClearValue depthClear {.depthStencil = {.depth = 1.f, .stencil = 0}};
    VkRenderPassBeginInfo renderPassBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mPrepassRenderpass,
        .framebuffer = mPrepassFramebuffer,
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = {
                .width = mWidth,
                .height = mHeight
            }
        },
        .clearValueCount = 1,
        .pClearValues = &depthClear
    };

    beginDebugLabel(commandBuffer, "Prepass");
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPrepassPipeline);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            mPrepassPipeline,
                            0, 1, &mCameraDs,
                            0, nullptr);

    for (const auto& [id, model] : mModels)
    {
        pfnCmdSetCullModeEXT(commandBuffer, model.cullMode);
        pfnCmdSetFrontFaceEXT(commandBuffer, model.frontFace);

        for (const auto& mesh : model.meshes)
        {
            if (model.drawOpaque(mesh))
                mesh.mesh.render(commandBuffer);
        }
    }

    vkCmdEndRenderPass(commandBuffer);
    endDebugLabel(commandBuffer);
}

void Renderer::executeSkyboxRenderpass(VkCommandBuffer commandBuffer)
{
    VkClearValue colorClear {.color = {
        mClearColor.r,
        mClearColor.g,
        mClearColor.b,
        mClearColor.a
    }};

    VkRenderPassBeginInfo renderPassBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mSkyboxRenderpass,
        .framebuffer = mSkyboxFramebuffer,
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = {
                .width = mWidth,
                .height = mHeight
            }
        },
        .clearValueCount = 1,
        .pClearValues = &colorClear
    };

    beginDebugLabel(commandBuffer, "Skybox Renderpass");
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    if (mRenderSkybox)
    {
        glm::mat4 proj = glm::perspective(glm::radians(mSkyboxFov),
                                          static_cast<float>(mWidth) / static_cast<float>(mHeight),
                                          *mCamera.nearPlane(),
                                          *mCamera.farPlane());
        proj[1][1] *= -1.f;

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mSkyboxPipeline);

        std::array<VkDescriptorSet, 2> descriptorSets {mCameraDs, mSkyboxDs};
        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                mSkyboxPipeline,
                                0, descriptorSets.size(), descriptorSets.data(),
                                0, nullptr);

        vkCmdPushConstants(commandBuffer,
                           mSkyboxPipeline,
                           VK_SHADER_STAGE_VERTEX_BIT,
                           0, sizeof(proj),
                           glm::value_ptr(proj));

        vkCmdDraw(commandBuffer, 36, 1, 0, 0);
    }

    vkCmdEndRenderPass(commandBuffer);
    endDebugLabel(commandBuffer);
}

void Renderer::executeSsaoResourcesRenderpass(VkCommandBuffer commandBuffer)
{
    std::array<VkClearValue, 3> clearValues {{
        {.color = {0.f, 0.f, 0.f, 0.f}}, // normals clear
        {.color = {0.f, 0.f, 0.f, 0.f}}, // view pos clear
        {.depthStencil = {.depth = 1.f, .stencil = 0}} // depth clear
    }};

    VkRenderPassBeginInfo renderPassBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mSsaoResourcesRenderpass,
        .framebuffer = mSsaoResourcesFramebuffer,
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = {
                .width = mWidth,
                .height = mHeight
            }
        },
        .clearValueCount = static_cast<uint32_t>(clearValues.size()),
        .pClearValues = clearValues.data()
    };

    beginDebugLabel(commandBuffer, "SSAO Resources");
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mSsaoResourcesPipeline);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            mSsaoResourcesPipeline,
                            0, 1, &mCameraDs,
                            0, nullptr);

    for (const auto& [id, model] : mModels)
    {
        pfnCmdSetCullModeEXT(commandBuffer, model.cullMode);
        pfnCmdSetFrontFaceEXT(commandBuffer, model.frontFace);

        for (const auto& mesh : model.meshes)
        {
            if (model.drawOpaque(mesh))
                mesh.mesh.render(commandBuffer);
        }
    }

    vkCmdEndRenderPass(commandBuffer);
    endDebugLabel(commandBuffer);
}

void Renderer::executeSsaoRenderpass(VkCommandBuffer commandBuffer)
{
    VkClearValue ssaoClear {.color = {1.f}};
    VkRenderPassBeginInfo renderPassBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mSsaoRenderpass,
        .framebuffer = mSsaoFramebuffer,
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = {.width = mWidth, .height = mHeight}
        },
        .clearValueCount = 1,
        .pClearValues = &ssaoClear
    };

    std::array<VkDescriptorSet, 2> descriptorSets {
        mCameraDs,
        mSsaoDs
    };

    struct {
        uint32_t ssaoKernelSize;
        float ssaoRadius;
        float ssaoIntensity;
        float ssaoBias;
        glm::vec2 screenSize;
        float noiseTextureSize;
    } pushConstants {static_cast<uint32_t>(mSsaoKernel.size()), mSsaoRadius, mSsaoIntensity,
        mSsaoDepthBias, glm::vec2(mWidth, mHeight), SsaoNoiseTextureSize
    };

    beginDebugLabel(commandBuffer, "SSAO Renderpass");
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mSsaoPipeline);

    vkCmdBindDescriptorSets(commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            mSsaoPipeline,
                            0, descriptorSets.size(), descriptorSets.data(),
                            0, nullptr);

    vkCmdPushConstants(commandBuffer,
                       mSsaoPipeline,
                       VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(pushConstants),
                       &pushConstants);

    if (mSsaoOn) vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
    endDebugLabel(commandBuffer);
}

void Renderer::executeSsaoBlurRenderpass(VkCommandBuffer commandBuffer)
{
    beginDebugLabel(commandBuffer, "SSAO Blur Renderpass");

    VkClearValue ssaoClear {.color = {1.f}};
    VkRenderPassBeginInfo renderPassBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mSsaoBlurRenderpass,
        .framebuffer = mSsaoBlurFramebuffer1,
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = {
                .width = mWidth,
                .height = mHeight
            }
        },
        .clearValueCount = 1,
        .pClearValues = &ssaoClear
    };

    glm::vec2 texelSize = 1.f / glm::vec2(mWidth, mHeight);

    { // blur horizontal
        glm::vec2 pushConstants[2] {
            texelSize,
            glm::vec2(1.f, 0.f)
        };

        vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mSsaoBlurPipeline);

        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                mSsaoBlurPipeline,
                                0, 1, &mSsaoTextureDs,
                                0, nullptr);

        vkCmdPushConstants(commandBuffer,
                           mSsaoBlurPipeline,
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(pushConstants),
                           pushConstants);

        if (mSsaoOn) vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer);
    }

    { // blur vertical
        renderPassBeginInfo.framebuffer = mSsaoBlurFramebuffer2;

        glm::vec2 pushConstants[2] {
            texelSize,
            glm::vec2(0.f, 1.f)
        };

        vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mSsaoBlurPipeline);

        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                mSsaoBlurPipeline,
                                0, 1, &mSsaoBlurTexture1Ds,
                                0, nullptr);

        vkCmdPushConstants(commandBuffer,
                           mSsaoBlurPipeline,
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(pushConstants),
                           pushConstants);

        if (mSsaoOn) vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer);
    }

    endDebugLabel(commandBuffer);
}

void Renderer::executeGenFrustumClustersRenderpass(VkCommandBuffer commandBuffer)
{
    beginDebugLabel(commandBuffer, "Gen Frustum Clusters");

    struct {
        alignas(16) glm::mat4 invProj;
        alignas(16) glm::uvec4 clusterGrid;
        alignas(8) glm::uvec2 screenSize;
    } pushConstants {
        glm::inverse(mCamera.projection()),
        glm::uvec4(mClusterGridSize, 0.),
        glm::uvec2(mWidth, mHeight)
    };

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mFrustumClusterGenPipeline);

    std::array<VkDescriptorSet, 2> ds {mCameraDs, mFrustumClusterGenDs};
    vkCmdBindDescriptorSets(commandBuffer,
                            VK_PIPELINE_BIND_POINT_COMPUTE,
                            mFrustumClusterGenPipelineLayout,
                            0, ds.size(), ds.data(),
                            0, nullptr);

    vkCmdPushConstants(commandBuffer,
                       mFrustumClusterGenPipelineLayout,
                       VK_SHADER_STAGE_COMPUTE_BIT,
                       0, sizeof(pushConstants),
                       &pushConstants);

    vkCmdDispatch(commandBuffer, mClusterGridSize.x, mClusterGridSize.y, mClusterGridSize.z);

    VkBufferMemoryBarrier clustersBufferMemoryBarrier {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
        .buffer = mVolumeClustersSSBO.getBuffer(),
        .offset = 0,
        .size = VK_WHOLE_SIZE
    };

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         0,
                         0, nullptr,
                         1, &clustersBufferMemoryBarrier,
                         0, nullptr);

    endDebugLabel(commandBuffer);
}

void Renderer::executeAssignLightsToClustersRenderpass(VkCommandBuffer commandBuffer)
{
    struct {
        alignas(16) glm::uvec4 clusterGrid;
        alignas(4) uint32_t pointLightCount;
    } pushConstants {
        glm::uvec4(mClusterGridSize, 0),
        static_cast<uint32_t>(mPointLights.size())
    };

    beginDebugLabel(commandBuffer, "Create Light List");
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, mAssignLightsToClustersPipeline);

    std::array<VkDescriptorSet, 2> ds {mCameraDs, mAssignLightsToClustersDs};
    vkCmdBindDescriptorSets(commandBuffer,
                            VK_PIPELINE_BIND_POINT_COMPUTE,
                            mAssignLightsToClustersPipelineLayout,
                            0, ds.size(), ds.data(),
                            0, nullptr);

    vkCmdPushConstants(commandBuffer,
                       mAssignLightsToClustersPipelineLayout,
                       VK_SHADER_STAGE_COMPUTE_BIT,
                       0, sizeof(pushConstants),
                       &pushConstants);

    vkCmdDispatch(commandBuffer, mClusterGridSize.x, mClusterGridSize.y, mClusterGridSize.z);

    VkBufferMemoryBarrier clustersBufferMemoryBarrier {
        .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
        .srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .buffer = mVolumeClustersSSBO.getBuffer(),
        .offset = 0,
        .size = VK_WHOLE_SIZE
    };

    vkCmdPipelineBarrier(commandBuffer,
                         VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                         VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                         0,
                         0, nullptr,
                         1, &clustersBufferMemoryBarrier,
                         0, nullptr);

    endDebugLabel(commandBuffer);
}

void Renderer::executeForwardRenderpass(VkCommandBuffer commandBuffer)
{
    VkRenderPassBeginInfo renderPassBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mForwardRenderpass,
        .framebuffer = mForwardPassFramebuffer,
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = {.width = mWidth, .height = mHeight}
        },
        .clearValueCount = 0,
        .pClearValues = nullptr
    };

    std::array<uint32_t, 10> pushConstants {
        static_cast<uint32_t>(mDirLights.size()),
        static_cast<uint32_t>(mPointLights.size()),
        static_cast<uint32_t>(mSpotLights.size()),
        static_cast<uint32_t>(mDebugNormals),
        mWidth,
        mHeight,
        mClusterGridSize.x,
        mClusterGridSize.y,
        mClusterGridSize.z,
        static_cast<uint32_t>(mEnableIblLighting)
    };

    beginDebugLabel(commandBuffer, "Forward Pass");

    { // opaque pass
        vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mOpaqueForwardPassPipeline);

        std::array<VkDescriptorSet, 3> ds {mCameraDs, mForwardShadingDs, mLightsDs};
        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                mOpaqueForwardPassPipeline,
                                0, ds.size(), ds.data(),
                                0, nullptr);

        vkCmdPushConstants(commandBuffer,
                           mOpaqueForwardPassPipeline,
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(uint32_t) * pushConstants.size(),
                           pushConstants.data());

        for (const auto& [id, model] : mModels)
        {
            pfnCmdSetCullModeEXT(commandBuffer, model.cullMode);
            pfnCmdSetFrontFaceEXT(commandBuffer, model.frontFace);

            for (const auto& mesh : model.meshes)
            {
                if (model.drawOpaque(mesh))
                {
                    uint32_t materialIndex = mesh.materialIndex;

                    model.bindMaterialUBO(commandBuffer, mOpaqueForwardPassPipeline, materialIndex, 3);
                    model.bindTextures(commandBuffer, mOpaqueForwardPassPipeline, materialIndex, 3);

                    mesh.mesh.render(commandBuffer);
                }
            }
        }

        vkCmdEndRenderPass(commandBuffer);
    }

    { // transparent pass
        vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mTransparentForwardPassPipeline);

        std::array<VkDescriptorSet, 3> ds {mCameraDs, mForwardShadingDs, mLightsDs};
        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                mTransparentForwardPassPipeline,
                                0, ds.size(), ds.data(),
                                0, nullptr);

        vkCmdPushConstants(commandBuffer,
                           mTransparentForwardPassPipeline,
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(uint32_t) * pushConstants.size(),
                           pushConstants.data());

        for (const auto& transparentMesh : mSortedTransparentMeshes)
        {
            vkCmdPushConstants(commandBuffer,
                               mTransparentForwardPassPipeline,
                               VK_SHADER_STAGE_VERTEX_BIT,
                               sizeof(glm::mat4), sizeof(glm::mat4),
                               glm::value_ptr(transparentMesh.model));

            mModels.at(transparentMesh.modelID).bindMaterialUBO(commandBuffer, mTransparentForwardPassPipeline, transparentMesh.mesh->materialIndex, 3);
            mModels.at(transparentMesh.modelID).bindTextures(commandBuffer, mTransparentForwardPassPipeline, transparentMesh.mesh->materialIndex, 3);

            VkDeviceSize offset = 0;
            VkBuffer vertexBuffer = transparentMesh.mesh->mesh.getVertexBuffer();
            VkBuffer indexBuffer = transparentMesh.mesh->mesh.getIndexBuffer();
            uint32_t indexCount = transparentMesh.mesh->mesh.indexCount();
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);
            vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0);
        }

        vkCmdEndRenderPass(commandBuffer);
    }

    endDebugLabel(commandBuffer);
}

void Renderer::executeBloomRenderpass(VkCommandBuffer commandBuffer)
{
    // 1. prefilter
    VkClearValue clearValue = {.color = {0.f, 0.f, 0.f, 0.f}};

    uint32_t w = mBloomMipChain.at(0).width;
    uint32_t h = mBloomMipChain.at(0).height;

    VkRenderPassBeginInfo captureBrightPixelsRenderPassBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mCaptureBrightPixelsRenderpass,
        .framebuffer = mCaptureBrightPixelsFramebuffer,
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = {
                .width = w,
                .height = h
            }
        },
        .clearValueCount = 1,
        .pClearValues = &clearValue
    };

    VkViewport viewport_ {
        .x = 0.f,
        .y = 0.f,
        .width = static_cast<float>(w),
        .height = static_cast<float>(h),
        .minDepth = 0.f,
        .maxDepth = 1.f
    };

    VkRect2D scissor_ {
        .offset = {.x = 0, .y = 0},
        .extent = {
            .width = w,
            .height = h
        }
    };

    struct {
        glm::vec2 texelSize;
        float threshold;
    } pushConstants1 {
        glm::vec2(1.f / static_cast<float>(w), 1.f / static_cast<float>(h)),
        mThreshold
    };

    beginDebugLabel(commandBuffer, "Capture Bright Pixels Pass");
    vkCmdBeginRenderPass(commandBuffer, &captureBrightPixelsRenderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport_);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor_);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mCaptureBrightPixelsPipeline);
    vkCmdBindDescriptorSets(commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            mCaptureBrightPixelsPipeline,
                            0, 1, &mColorResolve32Ds,
                            0, nullptr);
    vkCmdPushConstants(commandBuffer,
                       mCaptureBrightPixelsPipeline,
                       VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(pushConstants1),
                       &pushConstants1);
    if (mBloomOn) vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);
    endDebugLabel(commandBuffer);

    // 2. Mip chain downsampling
    beginDebugLabel(commandBuffer, "Bloom Mip Chain Downsampling Pass");

    for (uint32_t i = 1; i < BloomMipChainSize; ++i)
    {
        uint32_t width = mBloomMipChain.at(i).width;
        uint32_t height = mBloomMipChain.at(i).height;

        VkRenderPassBeginInfo mipChainDownsampleRenderpassInfo {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = mBloomDownsampleRenderpass,
            .framebuffer = mBloomDownsampleFramebuffers.at(i),
            .renderArea = {
                .offset = {.x = 0, .y = 0},
                .extent = {
                    .width = width,
                    .height = height
                }
            },
            .clearValueCount = 0,
            .pClearValues = nullptr
        };

        VkViewport viewport {
            .x = 0.f,
            .y = 0.f,
            .width = static_cast<float>(width),
            .height = static_cast<float>(height),
            .minDepth = 0.f,
            .maxDepth = 1.f
        };

        VkRect2D scissor {
            .offset = {.x = 0, .y = 0},
            .extent = {
                .width = width,
                .height = height
            }
        };

        struct {
            glm::vec2 texelSize;
            uint32_t mipLevel;
        } pushConstants {
            glm::vec2(1.f / static_cast<float>(width), 1.f / static_cast<float>(height)),
            i
        };

        vkCmdBeginRenderPass(commandBuffer, &mipChainDownsampleRenderpassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mBloomDownsamplePipeline);
        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                mBloomDownsamplePipeline,
                                0, 1, &mBloomMipChainDs.at(i - 1),
                                0, nullptr);
        vkCmdPushConstants(commandBuffer,
                           mBloomDownsamplePipeline,
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(pushConstants),
                           &pushConstants);
        if (mBloomOn) vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        vkCmdEndRenderPass(commandBuffer);
    }

    endDebugLabel(commandBuffer);

    // 3. Mip chain upsampling
    beginDebugLabel(commandBuffer, "Bloom Mip Chain Upsampling Pass");

    for (int32_t i = BloomMipChainSize - 2; i >= 0; --i)
    {
        uint32_t width = mBloomMipChain.at(i).width;
        uint32_t height = mBloomMipChain.at(i).height;

        VkRenderPassBeginInfo mipChainUpsampleRenderpassInfo {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = mBloomUpsampleRenderpass,
            .framebuffer = mBloomUpsampleFramebuffers.at(i),
            .renderArea = {
                .offset = {.x = 0, .y = 0},
                .extent = {
                    .width = width,
                    .height = height
                }
            },
            .clearValueCount = 0,
            .pClearValues = nullptr
        };

        VkViewport viewport {
            .x = 0.f,
            .y = 0.f,
            .width = static_cast<float>(width),
            .height = static_cast<float>(height),
            .minDepth = 0.f,
            .maxDepth = 1.f
        };

        VkRect2D scissor {
            .offset = {.x = 0, .y = 0},
            .extent = {
                .width = width,
                .height = height
            }
        };

        struct {
            glm::vec2 texelSize;
            float radius;
        } pushConstants {
            glm::vec2(1.f / static_cast<float>(width), 1.f / static_cast<float>(height)),
            mFilterRadius
        };

        vkCmdBeginRenderPass(commandBuffer, &mipChainUpsampleRenderpassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mBloomUpsamplePipeline);
        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                mBloomUpsamplePipeline,
                                0, 1, &mBloomMipChainDs.at(i + 1),
                                0, nullptr);
        vkCmdPushConstants(commandBuffer,
                           mBloomUpsamplePipeline,
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(pushConstants),
                           &pushConstants);
        if (mBloomOn) vkCmdDraw(commandBuffer, 3, 1, 0, 0);
        vkCmdEndRenderPass(commandBuffer);
    }

    endDebugLabel(commandBuffer);
    setViewport(commandBuffer);
}

void Renderer::executePostProcessingRenderpass(VkCommandBuffer commandBuffer)
{
    VkClearValue clearValue {.color = {1.f, 1.f, 1.f, 1.f}};

    VkRenderPassBeginInfo renderPassBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mPostProcessingRenderpass,
        .framebuffer = mPostProcessingFramebuffer,
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = {
                .width = mWidth,
                .height = mHeight
            }
        },
        .clearValueCount = 1,
        .pClearValues = &clearValue
    };

    beginDebugLabel(commandBuffer, "Post Processing Renderpass");
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPostProcessingPipeline);

    struct {
        uint32_t mHDROn;
        float exposure;
        float maxWhite;
        uint32_t tonemap;
        float bloomStrength;
    } pushConstants {
        static_cast<uint32_t>(mHDROn),
        mExposure,
        mMaxWhite,
        static_cast<uint32_t>(mTonemap),
        mBloomStrength
    };

    vkCmdBindDescriptorSets(commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            mPostProcessingPipeline,
                            0, 1, &mPostProcessingDs,
                            0, nullptr);

    vkCmdPushConstants(commandBuffer,
                       mPostProcessingPipeline,
                       VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(pushConstants),
                       &pushConstants);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
    endDebugLabel(commandBuffer);
}

void Renderer::executeWireframeRenderpass(VkCommandBuffer commandBuffer)
{
    if (!mWireframeOn)
        return;

    VkRenderPassBeginInfo renderPassBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mWireframeRenderpass,
        .framebuffer = mWireframeFramebuffer,
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = {
                .width = mWidth,
                .height = mHeight
            }
        },
        .clearValueCount = 0,
        .pClearValues = nullptr
    };

    glm::mat4 viewportMat = glm::identity<glm::mat4>();
    viewportMat[0][0] = static_cast<float>(mWidth) / 2.f;
    viewportMat[1][1] = static_cast<float>(mHeight) / 2.f;
    viewportMat[3][0] = viewportMat[0][0];
    viewportMat[3][1] = viewportMat[1][1];

    float pushConstants[5] {
        mWireframeColor[0],
        mWireframeColor[1],
        mWireframeColor[2],
        mWireframeColor[3],
        mWireframeWidth
    };

    beginDebugLabel(commandBuffer, "Wireframe");
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mWireframePipeline);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            mWireframePipeline,
                            0, 1, &mCameraDs,
                            0, nullptr);

    vkCmdPushConstants(commandBuffer,
                       mWireframePipeline,
                       VK_SHADER_STAGE_GEOMETRY_BIT,
                       0, sizeof(glm::mat4),
                       glm::value_ptr(viewportMat));

    vkCmdPushConstants(commandBuffer,
                       mWireframePipeline,
                       VK_SHADER_STAGE_FRAGMENT_BIT,
                       sizeof(glm::mat4), sizeof(pushConstants),
                       pushConstants);

    for (const auto& [id, model] : mModels)
    {
        pfnCmdSetCullModeEXT(commandBuffer, model.cullMode);
        pfnCmdSetFrontFaceEXT(commandBuffer, model.frontFace);

        for (const auto& mesh : model.meshes)
            if (model.drawOpaque(mesh))
                mesh.mesh.render(commandBuffer);
    }

    vkCmdEndRenderPass(commandBuffer);
    endDebugLabel(commandBuffer);
}

void Renderer::executeGridRenderpass(VkCommandBuffer commandBuffer)
{
    VkRenderPassBeginInfo renderPassBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mGridRenderpass,
        .framebuffer = mGridFramebuffer,
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = {
                .width = mWidth,
                .height = mHeight
            }
        },
        .clearValueCount = 0,
        .pClearValues = nullptr
    };

    beginDebugLabel(commandBuffer, "Grid");
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mGridPipeline);

    vkCmdBindDescriptorSets(commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            mGridPipeline,
                            0, 1, &mCameraDs,
                            0, nullptr);

    vkCmdPushConstants(commandBuffer,
                       mGridPipeline,
                       VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(GridData),
                       &mGridData);

    if (mRenderGrid) vkCmdDraw(commandBuffer, 6, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
    endDebugLabel(commandBuffer);
}

void Renderer::executeLightIconRenderpass(VkCommandBuffer commandBuffer)
{
    VkRenderPassBeginInfo renderPassBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mLightIconRenderpass,
        .framebuffer = mLightIconFramebuffer,
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = {
                .width = mWidth,
                .height = mHeight
            }
        },
    };

    getLightIconRenderData();

    beginDebugLabel(commandBuffer, "Light Icon Renderpass");
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mLightIconPipeline);

    vkCmdBindDescriptorSets(commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            mLightIconPipeline,
                            0, 1, &mCameraDs,
                            0, nullptr);

    bindTexture(commandBuffer, mLightIconPipeline, mDepthTexture, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, 1, 1);
    for (const auto& renderData : mLightIconRenderData)
    {
        bindTexture(commandBuffer, mLightIconPipeline, *renderData.icon, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, 1);
        vkCmdPushConstants(commandBuffer,
                           mLightIconPipeline,
                           VK_SHADER_STAGE_VERTEX_BIT,
                           0, sizeof(glm::vec3),
                           &renderData.pos);
        vkCmdDraw(commandBuffer, 6, 1, 0, 0);
    }

    vkCmdEndRenderPass(commandBuffer);
    endDebugLabel(commandBuffer);
}

void Renderer::setViewport(VkCommandBuffer commandBuffer)
{
    VkViewport viewport {
        .x = 0,
        .y = 0,
        .width = static_cast<float>(mWidth),
        .height = static_cast<float>(mHeight),
        .minDepth = 0.f,
        .maxDepth = 1.f
    };

    VkRect2D scissor {
        .offset = {.x = 0, .y = 0},
        .extent = {
            .width = mWidth,
            .height = mHeight
        }
    };

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void Renderer::updateCameraUBO()
{
    CameraRenderData renderData(mCamera.renderData());
    mCameraUBO.mapBufferMemory(0, sizeof(CameraRenderData), &renderData);
}

void Renderer::getLightIconRenderData()
{
    mLightIconRenderData.clear();
    for (const auto& [id, index] : mUuidToDirLightIndex)
        mLightIconRenderData.emplace_back(mSceneGraph->searchNode(id)->globalTransform()[3],
                                          &mDirLightIcon);
    for (const auto& [id, index] : mUuidToPointLightIndex)
        mLightIconRenderData.emplace_back(mSceneGraph->searchNode(id)->globalTransform()[3],
                                          &mPointLightIcon);
    for (const auto& [id, index] : mUuidToSpotLightIndex)
        mLightIconRenderData.emplace_back(mSceneGraph->searchNode(id)->globalTransform()[3],
                                          &mSpotLightIcon);

    std::sort(mLightIconRenderData.begin(), mLightIconRenderData.end(),
              [this] (const auto& a, const auto& b) {
                  return glm::length(a.pos - mCamera.position()) > glm::length(b.pos - mCamera.position());
              });
}

void Renderer::sortTransparentMeshes()
{
    mSortedTransparentMeshes.clear();

    std::vector<GraphNode*> stack(1, mSceneGraph->root());

    while (!stack.empty())
    {
        GraphNode* node = stack.back();
        stack.pop_back();

        for (auto child : node->children())
            stack.push_back(child);

        auto modelID = node->modelID();
        if (modelID.has_value() && !node->meshIDs().empty())
        {
            uuid32_t meshID = node->meshIDs().front();
            Mesh* mesh = mModels.at(*modelID).getMesh(meshID);

            if (mModels.at(*modelID).drawOpaque(*mesh))
                continue;

            glm::vec4 center = glm::vec4(mesh->center, 1.f);
            center = node->globalTransform() * center;

            glm::vec3 distVec = mCamera.position() - glm::vec3(center);

            mSortedTransparentMeshes.emplace(glm::length(distVec), *modelID, mesh, node->globalTransform());
        }
    }
}

void Renderer::bindTexture(VkCommandBuffer commandBuffer,
                           VkPipelineLayout pipelineLayout,
                           const VulkanTexture &texture,
                           VkImageLayout layout,
                           uint32_t binding,
                           uint32_t set)
{
    VkDescriptorImageInfo imageInfo(texture.vulkanSampler.sampler, texture.imageView, layout);

    VkWriteDescriptorSet writeDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = VK_NULL_HANDLE,
        .dstBinding = binding,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &imageInfo
    };

    pfnCmdPushDescriptorSet(commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout,
                            set,
                            1, &writeDescriptorSet);
}

void Renderer::updateImports()
{
    for (auto itr = mModelDataFutures.begin(); itr != mModelDataFutures.end();)
    {
        if (itr->wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            ModelLoader modelData = itr->get();

            if (modelData.success())
            {
                addModel(modelData);
            }

            itr = mModelDataFutures.erase(itr);
        }
        else
            ++itr;
    }
}

void Renderer::addModel(ModelLoader &modelData)
{
    Model model(mRenderDevice);

    model.id = UUIDRegistry::generateModelID();
    model.name = modelData.path.filename().string();
    model.path = modelData.path;
    model.root = std::move(modelData.root);
    model.materials = std::move(modelData.materials);
    model.materialNames = std::move(modelData.materialNames);

    addMeshes(model, modelData);
    addTextures(model, modelData);

    model.createMaterialsUBO();
    model.createTextureDescriptorSets(mSingleImageDsLayout);

    mModels.emplace(model.id, std::move(model));
}

void Renderer::addMeshes(Model &model, ModelLoader &modelData)
{
    for (const auto& meshData : modelData.meshes)
    {
        Mesh mesh {
            .meshID = UUIDRegistry::generateMeshID(),
            .name = meshData.name,
            .mesh {mRenderDevice, meshData.vertices, meshData.indices},
            .materialIndex = meshData.materialIndex,
            .center = meshData.center
        };

        model.meshes.push_back(std::move(mesh));
    }
}

void Renderer::addTextures(Model &model, ModelLoader &modelData)
{
    for (const auto& imageData : modelData.images)
    {
        TextureSpecification textureSpecification {
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .width = static_cast<uint32_t>(imageData.width),
            .height = static_cast<uint32_t>(imageData.height),
            .layerCount = 1,
            .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
            .imageUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                VK_IMAGE_USAGE_SAMPLED_BIT,
            .imageAspect = VK_IMAGE_ASPECT_COLOR_BIT,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .magFilter = imageData.magFilter,
            .minFilter = imageData.minFilter,
            .wrapS = imageData.wrapModeS,
            .wrapT = imageData.wrapModeT,
            .wrapR = imageData.wrapModeT,
            .generateMipMaps = true
        };

        VulkanTexture texture(mRenderDevice, textureSpecification, imageData.imageData.get());
        texture.setDebugName(imageData.name);

        Texture tex {
            .name = imageData.name,
            .vulkanTexture = std::move(texture)
        };

        model.textures.push_back(std::move(tex));
    }
}

void Renderer::updateSceneGraph()
{
    updateGraphNode(mSceneGraph->root()->type(), mSceneGraph->root());
}

void Renderer::updateGraphNode(NodeType type, GraphNode *node)
{
    if (!node) return;

    switch (type)
    {
        case NodeType::Empty: updateEmptyNode(node); break;
        case NodeType::Mesh: updateMeshNode(node); break;
        case NodeType::DirectionalLight: updateDirLightNode(node); break;
        case NodeType::PointLight: updatePointLightNode(node); break;
        case NodeType::SpotLight: updateSpotLightNode(node); break;
    }

    for (auto* child : node->children())
        updateGraphNode(child->type(), child);
}

void Renderer::updateEmptyNode(GraphNode *node)
{
    node->updateGlobalTransform();
}

void Renderer::updateMeshNode(GraphNode *node)
{
    if (node->updateGlobalTransform())
    {
        Model& model = mModels.at(node->modelID().value());

        for (uint32_t meshID : node->meshIDs())
        {
            InstancedMesh& mesh = model.getMesh(meshID)->mesh;
            mesh.updateInstance(node->id(), node->globalTransform());
        }
    }
}

void Renderer::updateDirLightNode(GraphNode *node)
{
    if (node->updateGlobalTransform())
    {
        index_t lightIndex = mUuidToDirLightIndex.at(node->id());
        DirectionalLight& light = mDirLights.at(lightIndex);

        glm::vec3 dummy;
        glm::vec3 orientation;
        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(node->globalTransform()),
                                              glm::value_ptr(dummy),
                                              glm::value_ptr(orientation),
                                              glm::value_ptr(dummy));

        light.direction = glm::vec4(calcLightDir(orientation), 0.0);

        updateDirLight(node->id());
    }
}

void Renderer::updatePointLightNode(GraphNode *node)
{
    if (node->updateGlobalTransform())
    {
        index_t lightIndex = mUuidToPointLightIndex.at(node->id());
        PointLight& light = mPointLights.at(lightIndex);

        glm::vec3 dummy;
        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(node->globalTransform()),
                                              glm::value_ptr(light.position),
                                              glm::value_ptr(dummy),
                                              glm::value_ptr(dummy));
        updatePointLight(node->id());
    }
}

void Renderer::updateSpotLightNode(GraphNode *node)
{
    if (node->updateGlobalTransform())
    {
        index_t lightIndex = mUuidToSpotLightIndex.at(node->id());
        SpotLight& light = mSpotLights.at(lightIndex);

        glm::vec3 dummy;
        glm::vec3 orientation;
        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(node->globalTransform()),
                                              glm::value_ptr(light.position),
                                              glm::value_ptr(orientation),
                                              glm::value_ptr(dummy));

        light.direction = glm::vec4(calcLightDir(orientation), 0.f);

        updateSpotLight(node->id());
    }
}

void Renderer::createColorTexture32MS()
{
    TextureSpecification specification {
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .width = mWidth,
        .height = mHeight,
        .layerCount = 1,
        .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
        .imageAspect = VK_IMAGE_ASPECT_COLOR_BIT,
        .samples = SampleCount,
        .magFilter = TextureMagFilter::Nearest,
        .minFilter = TextureMinFilter::Nearest,
        .wrapS = TextureWrap::ClampToEdge,
        .wrapT = TextureWrap::ClampToEdge,
        .generateMipMaps = false
    };

    mColorTexture32MS = VulkanTexture(mRenderDevice, specification);
    mColorTexture32MS.setDebugName("Renderer::mColorTexture32MS");

    specification.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    specification.samples = VK_SAMPLE_COUNT_1_BIT;
    mColorResolve32 = VulkanTexture(mRenderDevice, specification);
    mColorResolve32.setDebugName("Renderer::mColorResolve32");
}

void Renderer::createColorTexture8U()
{
    TextureSpecification specification {
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .width = mWidth,
        .height = mHeight,
        .layerCount = 1,
        .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                      VK_IMAGE_USAGE_SAMPLED_BIT,
        .imageAspect = VK_IMAGE_ASPECT_COLOR_BIT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .magFilter = TextureMagFilter::Nearest,
        .minFilter = TextureMinFilter::Nearest,
        .wrapS = TextureWrap::ClampToEdge,
        .wrapT = TextureWrap::ClampToEdge,
        .generateMipMaps = false
    };

    mColorTexture8U = VulkanTexture(mRenderDevice, specification);
    mColorTexture8U.setDebugName("Renderer::mColorTexture8U");
}

void Renderer::createDepthTextures()
{
    TextureSpecification specification {
        .format = VK_FORMAT_D32_SFLOAT,
        .width = mWidth,
        .height = mHeight,
        .layerCount = 1,
        .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
        .imageUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT |
            VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
        .imageAspect = VK_IMAGE_ASPECT_DEPTH_BIT,
        .samples = SampleCount,
        .magFilter = TextureMagFilter::Nearest,
        .minFilter = TextureMinFilter::Nearest,
        .wrapS = TextureWrap::ClampToEdge,
        .wrapT = TextureWrap::ClampToEdge,
        .generateMipMaps = false
    };

    mDepthTextureMS = VulkanTexture(mRenderDevice, specification);
    mDepthTextureMS.setDebugName("Renderer::mDepthTextureMS");

    specification.samples = VK_SAMPLE_COUNT_1_BIT;
    mDepthTexture = VulkanTexture(mRenderDevice, specification);
    mDepthTexture.setDebugName("Renderer::mDepthTexture");
}

void Renderer::createViewPosTexture()
{
    TextureSpecification specification{
        .format = VK_FORMAT_R16G16B16A16_SFLOAT,
        .width = mWidth,
        .height = mHeight,
        .layerCount = 1,
        .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                      VK_IMAGE_USAGE_SAMPLED_BIT |
                      VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
        .imageAspect = VK_IMAGE_ASPECT_COLOR_BIT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .magFilter = TextureMagFilter::Nearest,
        .minFilter = TextureMinFilter::Nearest,
        .wrapS = TextureWrap::ClampToEdge,
        .wrapT = TextureWrap::ClampToEdge,
        .generateMipMaps = false
    };

    mViewPosTexture = VulkanTexture(mRenderDevice, specification);
    mViewPosTexture.setDebugName("Renderer::mViewPosTexture");
}

void Renderer::createNormalTexture()
{
    TextureSpecification specification{
        .format = VK_FORMAT_R16G16B16A16_SFLOAT,
        .width = mWidth,
        .height = mHeight,
        .layerCount = 1,
        .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT |
            VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
        .imageAspect = VK_IMAGE_ASPECT_COLOR_BIT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .magFilter = TextureMagFilter::Nearest,
        .minFilter = TextureMinFilter::Nearest,
        .wrapS = TextureWrap::ClampToEdge,
        .wrapT = TextureWrap::ClampToEdge,
        .generateMipMaps = false
    };

    mNormalTexture = VulkanTexture(mRenderDevice, specification);
    mNormalTexture.setDebugName("Renderer::mNormalTexture");
}

void Renderer::createSsaoTextures()
{
    TextureSpecification specification {
        .format = VK_FORMAT_R32_SFLOAT,
        .width = mWidth,
        .height = mHeight,
        .layerCount = 1,
        .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
        .imageAspect = VK_IMAGE_ASPECT_COLOR_BIT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .magFilter = TextureMagFilter::Nearest,
        .minFilter = TextureMinFilter::Nearest,
        .wrapS = TextureWrap::ClampToEdge,
        .wrapT = TextureWrap::ClampToEdge,
        .generateMipMaps = false
    };

    mSsaoTexture = VulkanTexture(mRenderDevice, specification);
    mSsaoTexture.setDebugName("Renderer::mSsaoTexture");

    mSsaoBlurTexture1 = VulkanTexture(mRenderDevice, specification);
    mSsaoBlurTexture1.setDebugName("Renderer::mSsaoBlurTexture1");

    specification.magFilter = TextureMagFilter::Linear;
    specification.minFilter = TextureMinFilter::LinearMipmapNearest;
    mSsaoBlurTexture2 = VulkanTexture(mRenderDevice, specification);
    mSsaoBlurTexture2.setDebugName("Renderer::mSsaoBlurTexture2");
}

void Renderer::createSingleImageDsLayout()
{
    DsLayoutSpecification specification {
        .bindings = {
            binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
            },
        .debugName = "Renderer::mSingleImageDsLayout"
    };

    mSingleImageDsLayout = VulkanDsLayout(mRenderDevice, specification);
}

void Renderer::createCameraRenderDataDsLayout()
{
    DsLayoutSpecification specification {
        .bindings = {
            binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_ALL)
        },
        .debugName = "Renderer::mCameraRenderDataDsLayout"
    };

    mCameraRenderDataDsLayout = VulkanDsLayout(mRenderDevice, specification);
}

void Renderer::createMaterialsDsLayout()
{
    DsLayoutSpecification specification {
        .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR,
        .bindings = {
            binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
        },
        .debugName = "Renderer::mMaterialsDsLayout"
    };

    mMaterialsDsLayout = VulkanDsLayout(mRenderDevice, specification);
}

void Renderer::createSsaoDsLayout()
{
    DsLayoutSpecification specification {
        .bindings = {
            binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
        },
        .debugName = "Renderer::mSSAODsLayout"
    };

    mSSAODsLayout = VulkanDsLayout(mRenderDevice, specification);
}

void Renderer::createOitResourcesDsLayout()
{
    DsLayoutSpecification mOitResourcesDsLayoutSpecification {
        .bindings = {
            binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
        },
        .debugName = "Renderer::mOitResourcesDs"
    };

    mOitResourcesDsLayout = {mRenderDevice, mOitResourcesDsLayoutSpecification};
}

void Renderer::createSingleInputAttachmentDsLayout()
{
    DsLayoutSpecification mSingleInputAttachmentDsLayoutSpecification {
        .bindings = {
            binding(0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
        },
        .debugName = "Renderer::mSingleInputAttachmentDsLayout"
    };

    mSingleInputAttachmentDsLayout = {mRenderDevice, mSingleInputAttachmentDsLayoutSpecification};
}

void Renderer::createLightsDsLayout()
{
    std::array<VkDescriptorBindingFlags, 12> descriptorBindingFlags {};
    descriptorBindingFlags.at(9) = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT;
    descriptorBindingFlags.at(10) = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT;
    descriptorBindingFlags.at(11) = VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT;

    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT descriptorSetLayoutBindingFlagsCreateInfoExt {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO_EXT,
        .bindingCount = static_cast<uint32_t>(descriptorBindingFlags.size()),
        .pBindingFlags = descriptorBindingFlags.data()
    };

    DsLayoutSpecification specification {
        .pNext = &descriptorSetLayoutBindingFlagsCreateInfoExt,
        .bindings = {
            binding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(5, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(6, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(7, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(8, VK_DESCRIPTOR_TYPE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(9, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MaxShadowMapsPerType, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(10, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MaxShadowMapsPerType, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(11, VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, MaxShadowMapsPerType, VK_SHADER_STAGE_FRAGMENT_BIT),
        },
        .debugName = "Renderer::mLightsDsLayout"
    };

    mLightsDsLayout = {mRenderDevice, specification};
}

void Renderer::createFrustumClusterGenDsLayout()
{
    DsLayoutSpecification specification {
        .bindings = {
            binding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT),
            binding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT),
        },
        .debugName = "Renderer::mFrustumClusterGenDsLayout"
    };

    mFrustumClusterGenDsLayout = {mRenderDevice, specification};
}

void Renderer::createAssignLightsToClustersDsLayout()
{
    DsLayoutSpecification specification {
        .bindings = {
            binding(0, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT),
            binding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_COMPUTE_BIT),
        },
        .debugName = "Renderer::mAssignLightsToClustersDsLayout"
    };

    mAssignLightsToClustersDsLayout = {mRenderDevice, specification};
}

void Renderer::createForwardShadingDsLayout()
{
    DsLayoutSpecification specification {
        .bindings = {
            binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
        },
        .debugName = "Renderer::mForwardShadingDsLayout"
    };

    mForwardShadingDsLayout = {mRenderDevice, specification};
}

void Renderer::createPostProcessingDsLayout()
{
    DsLayoutSpecification specification {
        .bindings = {
            binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
        },
        .debugName = "Renderer::mPostProcessingDsLayout"
    };

    mPostProcessingDsLayout = {mRenderDevice, specification};
}

void Renderer::addDirShadowMap(const DirShadowData& shadowData)
{
    // add resources
    mDirShadowData.emplace_back(shadowData);
    mDirShadowMaps.emplace_back();
    createDirShadowMap(mDirShadowMaps.back(), mDirLights.size() - 1);

    // update buffer
    mDirShadowDataSSBO.update(0,
                              sizeof(DirShadowData) * mDirShadowData.size(),
                              mDirShadowData.data());
}

void Renderer::updateDirShadowsMaps()
{
    for (auto [id, index] : mUuidToDirLightIndex)
    {
        DirShadowData& dsd = mDirShadowData.at(index);
        DirectionalLight& light = mDirLights.at(index);

        std::vector<glm::vec2> cascadePlanes = calcCascadePlanes(*mCamera.nearPlane(), *mCamera.farPlane(), dsd.cascadeCount);

        for (uint32_t i = 0; i < dsd.cascadeCount; ++i)
        {
            dsd.viewProj[i] = calcMatrices(*mCamera.fov(),
                                           static_cast<float>(mWidth) / static_cast<float>(mHeight),
                                           cascadePlanes.at(i)[0],
                                           cascadePlanes.at(i)[1],
                                           mCamera.view(),
                                           light.direction,
                                           dsd.zScalar);
            dsd.cascadeDist[i] = cascadePlanes.at(i)[1];
        }
    }

    if (!mDirLights.empty())
        mDirShadowDataSSBO.update(0, sizeof(DirShadowData) * mDirLights.size(), mDirShadowData.data());
}

void Renderer::updateDirShadowMapImage(uuid32_t id)
{
    index_t i = mUuidToDirLightIndex.at(id);
    createDirShadowMap(mDirShadowMaps.at(i), i);
}

void Renderer::createDirShadowMap(DirShadowMap &shadowMap, index_t index)
{
    const DirShadowData& dsd = mDirShadowData.at(index);

    uint32_t resolution = dsd.resolution;
    uint32_t cascadeCount = dsd.cascadeCount;

    shadowMap.shadowMap = {
        mRenderDevice,
        VK_IMAGE_VIEW_TYPE_2D_ARRAY,
        VK_FORMAT_D32_SFLOAT,
        resolution,
        resolution,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        1, VK_SAMPLE_COUNT_1_BIT, cascadeCount
    };

    shadowMap.shadowMap.createLayerImageViews(VK_IMAGE_VIEW_TYPE_2D);

    // delete prev framebuffers
    for (auto fb : shadowMap.framebuffers)
        vkDestroyFramebuffer(mRenderDevice.device, fb, nullptr);

    // create framebuffers
    shadowMap.framebuffers.resize(cascadeCount);
    for (uint32_t i = 0; i < cascadeCount; ++i)
    {
        VkFramebufferCreateInfo framebufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = mDirShadowRenderpass,
            .attachmentCount = 1,
            .pAttachments = &shadowMap.shadowMap.layerImageViews.at(i),
            .width = resolution,
            .height = resolution,
            .layers = 1
        };

        VkResult result = vkCreateFramebuffer(mRenderDevice.device,
                                              &framebufferCreateInfo,
                                              nullptr,
                                              &shadowMap.framebuffers.at(i));
        vulkanCheck(result, "Failed to create framebuffer");
        setFramebufferDebugName(mRenderDevice,
                                shadowMap.framebuffers.at(i),
                                std::format("DirShadowMap::framebuffers.at({})", i));
    }

    // update ds
    VkDescriptorImageInfo imageInfo {
        .sampler = VK_NULL_HANDLE,
        .imageView = shadowMap.shadowMap.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
    };

    VkWriteDescriptorSet writeDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = mLightsDs,
        .dstBinding = 9,
        .dstArrayElement = index,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .pImageInfo = &imageInfo
    };

    vkUpdateDescriptorSets(mRenderDevice.device, 1, &writeDescriptorSet, 0, nullptr);
}

void Renderer::deleteDirShadowMap(uuid32_t id)
{
    index_t removeIndex = mUuidToDirLightIndex.at(id);
    index_t lastIndex = mDirLights.size() - 1;

    if (removeIndex != lastIndex)
    {
        // move options + shadow resources
        std::swap(mDirShadowData.at(removeIndex), mDirShadowData.at(lastIndex));
        std::swap(mDirShadowMaps.at(removeIndex), mDirShadowMaps.at(lastIndex));

        // update ds
        VkDescriptorImageInfo imageInfo {
            .sampler = VK_NULL_HANDLE,
            .imageView = mDirShadowMaps.at(removeIndex).shadowMap.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
        };

        VkWriteDescriptorSet writeDs {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = mLightsDs,
            .dstBinding = 9,
            .dstArrayElement = removeIndex,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .pImageInfo = &imageInfo
        };

        vkUpdateDescriptorSets(mRenderDevice.device, 1, &writeDs, 0, nullptr);
    }

    // delete last index
    for (auto& framebuffer : mDirShadowMaps.back().framebuffers)
        vkDestroyFramebuffer(mRenderDevice.device, framebuffer, nullptr);

    mDirShadowData.pop_back();
    mDirShadowMaps.pop_back();

    // update ssbo
    if (!mDirShadowData.empty())
        mDirShadowDataSSBO.update(0, sizeof(DirShadowData) * mDirShadowData.size(), mDirShadowData.data());
}

DirShadowData &Renderer::getDirShadowData(uuid32_t id)
{
    index_t i = mUuidToDirLightIndex.at(id);
    return mDirShadowData.at(i);
}

DirShadowMap &Renderer::getDirShadowMap(uuid32_t id)
{
    index_t i = mUuidToDirLightIndex.at(id);
    return mDirShadowMaps.at(i);
}

void Renderer::addPointShadowMap(const PointShadowData& shadowData)
{
    // add resources
    mPointShadowData.emplace_back(shadowData);
    mPointShadowMaps.emplace_back();
    createPointShadowMap(mPointShadowMaps.back(), mPointLights.size() - 1, mPointShadowData.back().resolution);

    // update buffer
    mPointShadowDataSSBO.update(0,
                               sizeof(PointShadowData) * mPointShadowData.size(),
                               mPointShadowData.data());
}

void Renderer::updatePointShadowMapData(uuid32_t id)
{
    index_t i = mUuidToPointLightIndex.at(id);
    mPointShadowDataSSBO.update(sizeof(PointShadowData) * i, sizeof(PointShadowData), &mPointShadowData.at(i));
}

void Renderer::updatePointShadowMapImage(uuid32_t id)
{
    index_t i = mUuidToPointLightIndex.at(id);
    uint32_t resolution =  mPointShadowData.at(i).resolution;

    // create new image
    createPointShadowMap(mPointShadowMaps.at(i), i, resolution);
}

void Renderer::createPointShadowMap(PointShadowMap &shadowMap, index_t index, uint32_t resolution)
{
    shadowMap.shadowMap = {
        mRenderDevice,
        VK_IMAGE_VIEW_TYPE_CUBE,
        VK_FORMAT_D32_SFLOAT,
        resolution,
        resolution,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        1, VK_SAMPLE_COUNT_1_BIT, 6,
        VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
    };

    shadowMap.shadowMap.createLayerImageViews(VK_IMAGE_VIEW_TYPE_2D);

    // create framebuffers
    for (uint32_t i = 0; i < 6; ++i)
    {
        vkDestroyFramebuffer(mRenderDevice.device, shadowMap.framebuffers[i], nullptr);
        VkFramebufferCreateInfo framebufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = mPointShadowRenderpass,
            .attachmentCount = 1,
            .pAttachments = &shadowMap.shadowMap.layerImageViews.at(i),
            .width = resolution,
            .height = resolution,
            .layers = 1
        };

        VkResult result = vkCreateFramebuffer(mRenderDevice.device,
                                              &framebufferCreateInfo,
                                              nullptr,
                                              &shadowMap.framebuffers[i]);
        vulkanCheck(result, "Failed to create framebuffer");
        setFramebufferDebugName(mRenderDevice,
                                shadowMap.framebuffers[i],
                                std::format("PointShadowMap::framebuffers[{}]", i));
    }

    // update ds
    VkDescriptorImageInfo imageInfo {
        .sampler = VK_NULL_HANDLE,
        .imageView = shadowMap.shadowMap.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
    };

    VkWriteDescriptorSet writeDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = mLightsDs,
        .dstBinding = 10,
        .dstArrayElement = index,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .pImageInfo = &imageInfo
    };

    vkUpdateDescriptorSets(mRenderDevice.device, 1, &writeDescriptorSet, 0, nullptr);
}

void Renderer::deletePointShadowMap(uuid32_t id)
{
    index_t removeIndex = mUuidToPointLightIndex.at(id);
    index_t lastIndex = mPointLights.size() - 1;

    if (removeIndex != lastIndex)
    {
        // move options + shadow resources
        std::swap(mPointShadowData.at(removeIndex), mPointShadowData.at(lastIndex));
        std::swap(mPointShadowMaps.at(removeIndex), mPointShadowMaps.at(lastIndex));

        // update ds
        VkDescriptorImageInfo imageInfo {
            .sampler = VK_NULL_HANDLE,
            .imageView = mPointShadowMaps.at(removeIndex).shadowMap.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
        };

        VkWriteDescriptorSet writeDs {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = mLightsDs,
            .dstBinding = 10,
            .dstArrayElement = removeIndex,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .pImageInfo = &imageInfo
        };

        vkUpdateDescriptorSets(mRenderDevice.device, 1, &writeDs, 0, nullptr);
    }

    // delete last index
    for (auto & framebuffer : mPointShadowMaps.back().framebuffers)
        vkDestroyFramebuffer(mRenderDevice.device, framebuffer, nullptr);

    mPointShadowData.pop_back();
    mPointShadowMaps.pop_back();

    // update ssbo
    if (!mPointShadowData.empty())
        mPointShadowDataSSBO.update(0, sizeof(PointShadowData) * mPointShadowData.size(), mPointShadowData.data());
}

PointShadowData &Renderer::getPointShadowData(uuid32_t id)
{
    index_t i = mUuidToPointLightIndex.at(id);
    return mPointShadowData.at(i);
}

PointShadowMap &Renderer::getPointShadowMap(uuid32_t id)
{
    index_t i = mUuidToPointLightIndex.at(id);
    return mPointShadowMaps.at(i);
}

void Renderer::addSpotShadowMap(const SpotShadowData& shadowData)
{
    // add resources
    mSpotShadowData.emplace_back(shadowData);
    mSpotShadowMaps.emplace_back();
    createSpotShadowMap(mSpotShadowMaps.back(), mSpotLights.size() - 1, mSpotShadowData.back().resolution);

    // update buffer
    mSpotShadowDataSSBO.update(0,
                               sizeof(SpotShadowData) * mSpotShadowData.size(),
                               mSpotShadowData.data());
}

void Renderer::updateSpotShadowMapData(uuid32_t id)
{
    index_t i = mUuidToSpotLightIndex.at(id);
    mSpotShadowDataSSBO.update(sizeof(SpotShadowData) * i, sizeof(SpotShadowData), &mSpotShadowData.at(i));
}

void Renderer::updateSpotShadowMapImage(uuid32_t id)
{
    index_t i = mUuidToSpotLightIndex.at(id);
    uint32_t resolution =  mSpotShadowData.at(i).resolution;

    // create new image
    createSpotShadowMap(mSpotShadowMaps.at(i), i, resolution);
}

void Renderer::createSpotShadowMap(SpotShadowMap& shadowMap, index_t index, uint32_t resolution)
{
    shadowMap.shadowMap = {
        mRenderDevice,
        VK_IMAGE_VIEW_TYPE_2D,
        VK_FORMAT_D32_SFLOAT,
        resolution,
        resolution,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_IMAGE_ASPECT_DEPTH_BIT
    };

    vkDestroyFramebuffer(mRenderDevice.device, shadowMap.framebuffer, nullptr);
    VkFramebufferCreateInfo framebufferCreateInfo {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = mSpotShadowRenderpass,
        .attachmentCount = 1,
        .pAttachments = &shadowMap.shadowMap.imageView,
        .width = resolution,
        .height = resolution,
        .layers = 1
    };

    VkResult result = vkCreateFramebuffer(mRenderDevice.device,
                                          &framebufferCreateInfo,
                                          nullptr,
                                          &shadowMap.framebuffer);
    vulkanCheck(result, "Failed to create framebuffer");
    setFramebufferDebugName(mRenderDevice,
                            shadowMap.framebuffer,
                            "SpotShadowMap::framebuffer");

    // update ds
    VkDescriptorImageInfo imageInfo {
        .sampler = VK_NULL_HANDLE,
        .imageView = shadowMap.shadowMap.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
    };

    VkWriteDescriptorSet writeDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = mLightsDs,
        .dstBinding = 11,
        .dstArrayElement = index,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
        .pImageInfo = &imageInfo
    };

    vkUpdateDescriptorSets(mRenderDevice.device, 1, &writeDescriptorSet, 0, nullptr);
}

void Renderer::deleteSpotShadowMap(uuid32_t id)
{
    index_t removeIndex = mUuidToSpotLightIndex.at(id);
    index_t lastIndex = mSpotLights.size() - 1;

    if (removeIndex != lastIndex)
    {
        // move options + shadow resources
        std::swap(mSpotShadowData.at(removeIndex), mSpotShadowData.at(lastIndex));
        std::swap(mSpotShadowMaps.at(removeIndex), mSpotShadowMaps.at(lastIndex));

        // update ds
        VkDescriptorImageInfo imageInfo {
            .sampler = VK_NULL_HANDLE,
            .imageView = mSpotShadowMaps.at(removeIndex).shadowMap.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
        };

        VkWriteDescriptorSet writeDs {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = mLightsDs,
            .dstBinding = 11,
            .dstArrayElement = removeIndex,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
            .pImageInfo = &imageInfo
        };

        vkUpdateDescriptorSets(mRenderDevice.device, 1, &writeDs, 0, nullptr);
    }

    // delete last index
    vkDestroyFramebuffer(mRenderDevice.device, mSpotShadowMaps.back().framebuffer, nullptr);
    mSpotShadowData.pop_back();
    mSpotShadowMaps.pop_back();

    // update ssbo
    if (!mSpotShadowData.empty())
        mSpotShadowDataSSBO.update(0, sizeof(SpotShadowData) * mSpotShadowData.size(), mSpotShadowData.data());
}

SpotShadowData &Renderer::getSpotShadowData(uuid32_t id)
{
    index_t i = mUuidToSpotLightIndex.at(id);
    return mSpotShadowData.at(i);
}

SpotShadowMap &Renderer::getSpotShadowMap(uuid32_t id)
{
    index_t i = mUuidToSpotLightIndex.at(id);
    return mSpotShadowMaps.at(i);
}

void Renderer::createShadowMapBuffers()
{
    mDirShadowDataSSBO = {mRenderDevice, MaxDirLights * sizeof(DirShadowData), BufferType::Storage, MemoryType::Device};
    mPointShadowDataSSBO = {mRenderDevice, MaxDirLights * sizeof(PointShadowData), BufferType::Storage, MemoryType::Device};
    mSpotShadowDataSSBO = {mRenderDevice, MaxDirLights * sizeof(SpotShadowData), BufferType::Storage, MemoryType::Device};

    mDirShadowDataSSBO.setDebugName("Renderer::mDirShadowDataSSBO");
    mPointShadowDataSSBO.setDebugName("Renderer::mPointShadowDataSSBO");
    mSpotShadowDataSSBO.setDebugName("Renderer::mSpotShadowDataSSBO");
}

void Renderer::createShadowMapSamplers()
{
    mDirShadowMapSampler = createSampler(mRenderDevice,
                                          TextureMagFilter::Nearest,
                                          TextureMinFilter::Nearest,
                                          TextureWrap::ClampToBorder,
                                          TextureWrap::ClampToBorder,
                                          TextureWrap::ClampToBorder);
    setVulkanObjectDebugName(mRenderDevice,
                             VK_OBJECT_TYPE_SAMPLER,
                             "Renderer::mDirShadowMapSampler",
                             mDirShadowMapSampler.sampler);

    mPointShadowMapSampler = createSampler(mRenderDevice,
                                           TextureMagFilter::Linear,
                                           TextureMinFilter::Linear,
                                           TextureWrap::ClampToEdge,
                                           TextureWrap::ClampToEdge,
                                           TextureWrap::ClampToEdge);
    setVulkanObjectDebugName(mRenderDevice,
                             VK_OBJECT_TYPE_SAMPLER,
                             "Renderer::mPointShadowMapSampler",
                             mPointShadowMapSampler.sampler);

    mSpotShadowMapSampler = createSampler(mRenderDevice,
                                          TextureMagFilter::Nearest,
                                          TextureMinFilter::Nearest,
                                          TextureWrap::ClampToBorder,
                                          TextureWrap::ClampToBorder,
                                          TextureWrap::ClampToBorder);
    setVulkanObjectDebugName(mRenderDevice,
                             VK_OBJECT_TYPE_SAMPLER,
                             "Renderer::mSpotShadowMapSampler",
                             mSpotShadowMapSampler.sampler);
}

void Renderer::createDirShadowRenderpass()
{
    VkAttachmentDescription attachment {
        .format = VK_FORMAT_D32_SFLOAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
    };

    VkAttachmentReference attachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .pDepthStencilAttachment = &attachmentRef
    };

    VkSubpassDependency dependency {
        .srcSubpass = 0,
        .dstSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
    };

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mDirShadowRenderpass);
    vulkanCheck(result, "Failed to create renderpass.");
    setRenderpassDebugName(mRenderDevice, mDirShadowRenderpass, "Renderer::mDirShadowRenderpass");
}

void Renderer::createDirShadowPipeline()
{
    PipelineSpecification specification {
        .shaderStages = {
            .vertShaderPath = "shaders/gen_dir_shadow_map.vert.spv",
            .fragShaderPath = "shaders/gen_dir_shadow_map.frag.spv"
        },
        .vertexInput = {
            .bindings = InstancedMesh::bindingDescriptionsInstanced(),
            .attributes = InstancedMesh::attributeDescriptionsInstanced()
        },
        .inputAssembly = {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        },
        .tesselation = {
            .patchControlUnits = 0
        },
        .rasterization = {
            .rasterizerDiscardPrimitives = false,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .lineWidth = 1.f
        },
        .multisampling = {
            .samples = VK_SAMPLE_COUNT_1_BIT
        },
        .depthStencil = {
            .enableDepthTest = VK_TRUE,
            .enableDepthWrite = VK_TRUE,
            .depthCompareOp = VK_COMPARE_OP_LESS
        },
        .blendStates = {
            {.enable = false},
            {.enable = false}
        },
        .dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
            VK_DYNAMIC_STATE_CULL_MODE_EXT,
            VK_DYNAMIC_STATE_FRONT_FACE_EXT
        },
        .pipelineLayout = {
            .pushConstantRanges = {
                {
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                    .offset = 0,
                    .size = sizeof(glm::mat4)
                }
            }
        },
        .renderPass = mDirShadowRenderpass,
        .subpassIndex = 0,
        .debugName = "Renderer::mDirShadowPipeline"
    };

    mDirShadowPipeline = {mRenderDevice, specification};
}

void Renderer::createSpotShadowRenderpass()
{
    VkAttachmentDescription attachment {
        .format = VK_FORMAT_D32_SFLOAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
    };

    VkAttachmentReference attachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .pDepthStencilAttachment = &attachmentRef
    };

    VkSubpassDependency dependency {
        .srcSubpass = 0,
        .dstSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
    };

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mSpotShadowRenderpass);
    vulkanCheck(result, "Failed to create renderpass.");
    setRenderpassDebugName(mRenderDevice, mSpotShadowRenderpass, "Renderer::mSpotShadowRenderpass");
}

void Renderer::createSpotShadowPipeline()
{
    PipelineSpecification specification {
        .shaderStages = {
            .vertShaderPath = "shaders/gen_spot_shadow_map.vert.spv",
            .fragShaderPath = "shaders/gen_spot_shadow_map.frag.spv"
        },
        .vertexInput = {
            .bindings = InstancedMesh::bindingDescriptionsInstanced(),
            .attributes = InstancedMesh::attributeDescriptionsInstanced()
        },
        .inputAssembly = {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        },
        .tesselation = {
            .patchControlUnits = 0
        },
        .rasterization = {
            .rasterizerDiscardPrimitives = false,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .lineWidth = 1.f
        },
        .multisampling = {
            .samples = VK_SAMPLE_COUNT_1_BIT
        },
        .depthStencil = {
            .enableDepthTest = VK_TRUE,
            .enableDepthWrite = VK_TRUE,
            .depthCompareOp = VK_COMPARE_OP_LESS
        },
        .blendStates = {
            {.enable = false},
            {.enable = false}
        },
        .dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
            VK_DYNAMIC_STATE_CULL_MODE_EXT,
            VK_DYNAMIC_STATE_FRONT_FACE_EXT
        },
        .pipelineLayout = {
            .pushConstantRanges = {
                {
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                    .offset = 0,
                    .size = sizeof(glm::mat4)
                }

            }
        },
        .renderPass = mSpotShadowRenderpass,
        .subpassIndex = 0,
        .debugName = "Renderer::mSpotShadowPipeline"
    };

    mSpotShadowPipeline = {mRenderDevice, specification};
}

void Renderer::createPointShadowRenderpass()
{
    VkAttachmentDescription attachment {
        .format = VK_FORMAT_D32_SFLOAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
    };

    VkAttachmentReference attachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .pDepthStencilAttachment = &attachmentRef
    };

    VkSubpassDependency dependency {
        .srcSubpass = 0,
        .dstSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
    };

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mPointShadowRenderpass);
    vulkanCheck(result, "Failed to create renderpass.");
    setRenderpassDebugName(mRenderDevice, mPointShadowRenderpass, "Renderer::mPointShadowRenderpass");
}

void Renderer::createPointShadowPipeline()
{
    PipelineSpecification specification {
        .shaderStages = {
            .vertShaderPath = "shaders/gen_point_shadow_map.vert.spv",
            .fragShaderPath = "shaders/gen_point_shadow_map.frag.spv"
        },
        .vertexInput = {
            .bindings = InstancedMesh::bindingDescriptionsInstanced(),
            .attributes = InstancedMesh::attributeDescriptionsInstanced()
        },
        .inputAssembly = {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        },
        .tesselation = {
            .patchControlUnits = 0
        },
        .rasterization = {
            .rasterizerDiscardPrimitives = false,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .lineWidth = 1.f
        },
        .multisampling = {
            .samples = VK_SAMPLE_COUNT_1_BIT
        },
        .depthStencil = {
            .enableDepthTest = VK_TRUE,
            .enableDepthWrite = VK_TRUE,
            .depthCompareOp = VK_COMPARE_OP_LESS
        },
        .blendStates = {
            {.enable = false},
            {.enable = false}
        },
        .dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
            VK_DYNAMIC_STATE_CULL_MODE_EXT,
            VK_DYNAMIC_STATE_FRONT_FACE_EXT
        },
        .pipelineLayout = {
            .pushConstantRanges = {
                {
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                    .offset = 0,
                    .size = sizeof(glm::mat4)
                },
                {
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .offset = sizeof(glm::mat4),
                    .size = sizeof(glm::vec4) + sizeof(float) * 2
                }
            }
        },
        .renderPass = mSpotShadowRenderpass,
        .subpassIndex = 0,
        .debugName = "Renderer::mPointShadowPipeline"
    };

    mPointShadowPipeline = {mRenderDevice, specification};
}

void Renderer::createPrepassRenderpass()
{
    VkAttachmentDescription depthAttachment {
        .format = mDepthTextureMS.format,
        .samples = SampleCount,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference depthAttachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .pDepthStencilAttachment = &depthAttachmentRef
    };

    VkSubpassDependency dependency {
        .srcSubpass = 0,
        .dstSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
            VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
        .srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
    };

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &depthAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mPrepassRenderpass);
    vulkanCheck(result, "Failed to create renderpass.");
    setRenderpassDebugName(mRenderDevice, mPrepassRenderpass, "Renderer::mPrepassRenderpass");
}

void Renderer::createPrepassFramebuffer()
{
    vkDestroyFramebuffer(mRenderDevice.device, mPrepassFramebuffer, nullptr);

    VkFramebufferCreateInfo framebufferCreateInfo {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = mPrepassRenderpass,
        .attachmentCount = 1,
        .pAttachments = &mDepthTextureMS.imageView,
        .width = mWidth,
        .height = mHeight,
        .layers = 1
    };

    VkResult result = vkCreateFramebuffer(mRenderDevice.device, &framebufferCreateInfo, nullptr, &mPrepassFramebuffer);
    vulkanCheck(result, "Failed to create framebuffer.");
    setFramebufferDebugName(mRenderDevice, mPrepassFramebuffer, "Renderer::mPrepassFramebuffer");
}

void Renderer::createPrepassPipeline()
{
    PipelineSpecification specification {
        .shaderStages = {
            .vertShaderPath = "shaders/prepass.vert.spv",
            .fragShaderPath = "shaders/prepass.frag.spv"
        },
        .vertexInput = {
            .bindings = InstancedMesh::bindingDescriptionsInstanced(),
            .attributes = InstancedMesh::attributeDescriptionsInstanced()
        },
        .inputAssembly = {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        },
        .tesselation = {
            .patchControlUnits = 0
        },
        .rasterization = {
            .rasterizerDiscardPrimitives = false,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .lineWidth = 1.f
        },
        .multisampling = {
            .samples = SampleCount
        },
        .depthStencil = {
            .enableDepthTest = VK_TRUE,
            .enableDepthWrite = VK_TRUE,
            .depthCompareOp = VK_COMPARE_OP_LESS
        },
        .blendStates = {
            {.enable = false},
            {.enable = false}
        },
        .dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
            VK_DYNAMIC_STATE_CULL_MODE_EXT,
            VK_DYNAMIC_STATE_FRONT_FACE_EXT
        },
        .pipelineLayout = {.dsLayouts = {mCameraRenderDataDsLayout}},
        .renderPass = mPrepassRenderpass,
        .subpassIndex = 0,
        .debugName = "Renderer::mPrepassPipeline"
    };

    mPrepassPipeline = VulkanGraphicsPipeline(mRenderDevice, specification);
}

void Renderer::createVolumeClusterSSBO()
{
    uint32_t clusterCount = mClusterGridSize.x * mClusterGridSize.y * mClusterGridSize.z;
    mVolumeClustersSSBO = {mRenderDevice,
                           sizeof(Cluster) * clusterCount,
                           BufferType::Storage,
                           MemoryType::Device};
    mVolumeClustersSSBO.setDebugName("Renderer::mVolumeClustersSSBO");
}

void Renderer::createFrustumClusterGenPipelineLayout()
{
    VkPushConstantRange pushConstantRange {
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = sizeof(glm::mat4) + sizeof(glm::vec4) * 2
    };

    std::array<VkDescriptorSetLayout, 2> setLayouts {
        mCameraRenderDataDsLayout,
        mFrustumClusterGenDsLayout
    };

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(setLayouts.size()),
        .pSetLayouts = setLayouts.data(),
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange
    };

    VkResult result = vkCreatePipelineLayout(mRenderDevice.device,
                                             &pipelineLayoutCreateInfo,
                                             nullptr,
                                             &mFrustumClusterGenPipelineLayout);
    vulkanCheck(result, "Failed to create pipeline layout.");

    setVulkanObjectDebugName(mRenderDevice,
                             VK_OBJECT_TYPE_PIPELINE_LAYOUT,
                             "Renderer::mFrustumClusterGenPipelineLayout",
                                 mFrustumClusterGenPipelineLayout);
}

void Renderer::createFrustumClusterGenPipeline()
{
    VulkanShaderModule shaderModule(mRenderDevice, "shaders/gen_frustum_clusters.comp.spv");

    VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = shaderModule,
        .pName = "main"
    };

    VkComputePipelineCreateInfo computePipelineCreateInfo {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = pipelineShaderStageCreateInfo,
        .layout = mFrustumClusterGenPipelineLayout
    };

    VkResult result = vkCreateComputePipelines(mRenderDevice.device,
                                               VK_NULL_HANDLE,
                                               1,
                                               &computePipelineCreateInfo,
                                               nullptr,
                                               &mFrustumClusterGenPipeline);
    vulkanCheck(result, "Failed to create compute pipeline.");

    setVulkanObjectDebugName(mRenderDevice,
                             VK_OBJECT_TYPE_PIPELINE,
                             "Renderer::mFrustumClusterGenPipeline",
                             mFrustumClusterGenPipeline);
}

void Renderer::createAssignLightsToClustersPipelineLayout()
{
    VkPushConstantRange pushConstantRange {
        .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
        .offset = 0,
        .size = sizeof(glm::vec4) * 2
    };

    std::array<VkDescriptorSetLayout, 2> dsLayouts {
        mCameraRenderDataDsLayout,
        mAssignLightsToClustersDsLayout
    };

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(dsLayouts.size()),
        .pSetLayouts = dsLayouts.data(),
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange
    };

    VkResult result = vkCreatePipelineLayout(mRenderDevice.device,
                                             &pipelineLayoutCreateInfo,
                                             nullptr,
                                             &mAssignLightsToClustersPipelineLayout);
    vulkanCheck(result, "Failed to create pipeline layout.");

    setVulkanObjectDebugName(mRenderDevice,
                             VK_OBJECT_TYPE_PIPELINE_LAYOUT,
                             "Renderer::mAssignLightsToClustersPipelineLayout",
                             mAssignLightsToClustersPipelineLayout);
}

void Renderer::createAssignLightsToClustersPipeline()
{
    VulkanShaderModule shaderModule(mRenderDevice, "shaders/assign_lights_to_clusters.comp.spv");

    VkPipelineShaderStageCreateInfo pipelineShaderStageCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = shaderModule,
        .pName = "main"
    };

    VkComputePipelineCreateInfo computePipelineCreateInfo {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = pipelineShaderStageCreateInfo,
        .layout = mAssignLightsToClustersPipelineLayout
    };

    VkResult result = vkCreateComputePipelines(mRenderDevice.device,
                                               VK_NULL_HANDLE,
                                               1,
                                               &computePipelineCreateInfo,
                                               nullptr,
                                               &mAssignLightsToClustersPipeline);
    vulkanCheck(result, "Failed to create compute pipeline.");

    setVulkanObjectDebugName(mRenderDevice,
                             VK_OBJECT_TYPE_PIPELINE,
                             "Renderer::mAssignLightsToClustersPipeline",
                             mAssignLightsToClustersPipeline);
}

void Renderer::createSkyboxRenderpass()
{
    VkAttachmentDescription colorAttachment {
        .format = mColorTexture32MS.format,
        .samples = SampleCount,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentDescription depthAttachment {
        .format = mDepthTextureMS.format,
        .samples = SampleCount,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    std::array<VkAttachmentDescription, 2> attachments {{
        colorAttachment,
        depthAttachment
    }};

    VkAttachmentReference colorAttachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference depthAttachmentRef {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pDepthStencilAttachment = &depthAttachmentRef
    };

    VkSubpassDependency dependency {
        .srcSubpass = 0,
        .dstSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
    };

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mSkyboxRenderpass);
    vulkanCheck(result, "Failed to create renderpass.");
    setRenderpassDebugName(mRenderDevice, mSkyboxRenderpass, "Renderer::mSkyboxRenderpass");
}

void Renderer::createSkyboxFramebuffer()
{
    vkDestroyFramebuffer(mRenderDevice.device, mSkyboxFramebuffer, nullptr);

    std::array<VkImageView, 2> imageViews {
        mColorTexture32MS.imageView,
        mDepthTextureMS.imageView,
    };

    VkFramebufferCreateInfo framebufferCreateInfo {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = mSkyboxRenderpass,
        .attachmentCount = static_cast<uint32_t>(imageViews.size()),
        .pAttachments = imageViews.data(),
        .width = mWidth,
        .height = mHeight,
        .layers = 1
    };

    VkResult result = vkCreateFramebuffer(mRenderDevice.device, &framebufferCreateInfo, nullptr, &mSkyboxFramebuffer);
    vulkanCheck(result, "Failed to create framebuffer.");
    setFramebufferDebugName(mRenderDevice, mSkyboxFramebuffer, "Renderer::mSkyboxFramebuffer");
}

void Renderer::createSkyboxPipeline()
{
    PipelineSpecification specification {
        .shaderStages = {
            .vertShaderPath = "shaders/skybox.vert.spv",
            .fragShaderPath = "shaders/skybox.frag.spv"
        },
        .inputAssembly = {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        },
        .tesselation = {
            .patchControlUnits = 0
        },
        .rasterization = {
            .rasterizerDiscardPrimitives = false,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE,
            .lineWidth = 1.f
        },
        .multisampling = {
            .samples = SampleCount
        },
        .depthStencil = {
            .enableDepthTest = VK_TRUE,
            .enableDepthWrite = VK_FALSE,
            .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL
        },
        .blendStates = {
            {.enable = false}
        },
        .dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        },
        .pipelineLayout = {
            .dsLayouts = {
                mCameraRenderDataDsLayout,
                mSingleImageDsLayout
            },
            .pushConstantRanges =  {
                {
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                    .offset = 0,
                    .size = sizeof(glm::mat4)
                }
            }
        },
        .renderPass = mSkyboxRenderpass,
        .subpassIndex = 0,
        .debugName = "Renderer::mSkyboxPipeline"
    };

    mSkyboxPipeline = VulkanGraphicsPipeline(mRenderDevice, specification);
}

void Renderer::createSsaoKernel(uint32_t sampleCount)
{
    mSsaoKernel.resize(sampleCount);

    for (size_t i = 0, count = mSsaoKernel.size(); i < count; ++i)
    {
        mSsaoKernel.at(i) = glm::vec4 {
            mSsaoDistribution(mSsaoRandomEngine) * 2.f - 1.f, // -> [-1, 1]
            mSsaoDistribution(mSsaoRandomEngine) * 2.f - 1.f, // -> [-1, 1]
            mSsaoDistribution(mSsaoRandomEngine), // -> [0, 1]
            0.f
        };

        mSsaoKernel.at(i) = glm::normalize(mSsaoKernel.at(i));
        mSsaoKernel.at(i) *= mSsaoDistribution(mSsaoRandomEngine);

        float scale = static_cast<float>(i) / static_cast<float>(count);
        scale = glm::lerp(0.1f, 1.f, glm::pow(scale, 2.f));
        mSsaoKernel.at(i) *= scale;
    }
}

void Renderer::createSsaoKernelSSBO()
{
    VkDeviceSize bufferSize = sizeof(glm::vec4) * MaxSsaoKernelSamples;
    mSsaoKernelSSBO = {mRenderDevice, bufferSize, BufferType::Storage, MemoryType::Device};
    mSsaoKernelSSBO.setDebugName("Renderer::mSsaoKernelSSBO");
}

void Renderer::updateSsaoKernelSSBO()
{
    VkDeviceSize kernelSize = sizeof(glm::vec4) * mSsaoKernel.size();
    mSsaoKernelSSBO.update(0, kernelSize, mSsaoKernel.data());
}

void Renderer::createSsaoNoiseTexture()
{
    std::vector<glm::vec4> randomVectors(glm::pow(SsaoNoiseTextureSize, 2));
    for (auto& randomVec : randomVectors)
    {
        randomVec = glm::vec4 {
            mSsaoDistribution(mSsaoRandomEngine) * 2.f - 1.f, // -> [-1, 1]
            mSsaoDistribution(mSsaoRandomEngine) * 2.f - 1.f, // -> [-1, 1]
            0.f,
            0.f
        };
    }

    TextureSpecification specification {
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .width = SsaoNoiseTextureSize,
        .height = SsaoNoiseTextureSize,
        .layerCount = 1,
        .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
        .imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                      VK_IMAGE_USAGE_SAMPLED_BIT,
        .imageAspect = VK_IMAGE_ASPECT_COLOR_BIT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .magFilter = TextureMagFilter::Nearest,
        .minFilter = TextureMinFilter::Nearest,
        .wrapS = TextureWrap::Repeat,
        .wrapT = TextureWrap::Repeat,
        .generateMipMaps = false
    };

    mSsaoNoiseTexture = {mRenderDevice, specification, randomVectors.data()};
    mSsaoNoiseTexture.setDebugName("Renderer::mSsaoNoiseTexture");
}

void Renderer::createSsaoResourcesRenderpass()
{
    VkAttachmentDescription normalsAttachment {
        .format = mNormalTexture.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkAttachmentDescription viewPosAttachment {
        .format = mViewPosTexture.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkAttachmentDescription depthAttachment {
        .format = mDepthTexture.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
    };

    std::array<VkAttachmentDescription, 3> attachments {{
        normalsAttachment,
        viewPosAttachment,
        depthAttachment
    }};

    VkAttachmentReference normalsAttachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference viewPosAttachmentRef {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference depthAttachmentRef {
        .attachment = 2,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    std::array<VkAttachmentReference, 2> colorAttachmentsRef {
        normalsAttachmentRef,
        viewPosAttachmentRef
    };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = static_cast<uint32_t>(colorAttachmentsRef.size()),
        .pColorAttachments = colorAttachmentsRef.data(),
        .pDepthStencilAttachment = &depthAttachmentRef
    };

    VkSubpassDependency dependency {
        .srcSubpass = 0,
        .dstSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT |
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
            VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT |
            VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
    };

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mSsaoResourcesRenderpass);
    vulkanCheck(result, "Failed to create renderpass.");
    setRenderpassDebugName(mRenderDevice, mSsaoResourcesRenderpass, "Renderer::mSsaoResourcesRenderpass");
}

void Renderer::createSsaoResourcesFramebuffer()
{
    vkDestroyFramebuffer(mRenderDevice.device, mSsaoResourcesFramebuffer, nullptr);

    std::array<VkImageView, 3> imageViews {
        mNormalTexture.imageView,
        mViewPosTexture.imageView,
        mDepthTexture.imageView
    };

    VkFramebufferCreateInfo framebufferCreateInfo {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = mSsaoResourcesRenderpass,
        .attachmentCount = static_cast<uint32_t>(imageViews.size()),
        .pAttachments = imageViews.data(),
        .width = mWidth,
        .height = mHeight,
        .layers = 1
    };

    VkResult result = vkCreateFramebuffer(mRenderDevice.device, &framebufferCreateInfo, nullptr, &mSsaoResourcesFramebuffer);
    vulkanCheck(result, "Failed to create framebuffer.");
    setFramebufferDebugName(mRenderDevice, mSsaoResourcesFramebuffer, "Renderer::mSsaoResourcesFramebuffer");
}

void Renderer::createSsaoResourcesPipeline()
{
    PipelineSpecification specification {
        .shaderStages = {
            .vertShaderPath = "shaders/ssao_resources.vert.spv",
            .fragShaderPath = "shaders/ssao_resources.frag.spv"
        },
        .vertexInput = {
            .bindings = InstancedMesh::bindingDescriptionsInstanced(),
            .attributes = InstancedMesh::attributeDescriptionsInstanced()
        },
        .inputAssembly = {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        },
        .tesselation = {
            .patchControlUnits = 0
        },
        .rasterization = {
            .rasterizerDiscardPrimitives = false,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .lineWidth = 1.f
        },
        .multisampling = {
            .samples = VK_SAMPLE_COUNT_1_BIT
        },
        .depthStencil = {
            .enableDepthTest = VK_TRUE,
            .enableDepthWrite = VK_TRUE,
            .depthCompareOp = VK_COMPARE_OP_LESS
        },
        .blendStates = {
            {.enable = false},
            {.enable = false}
        },
        .dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
            VK_DYNAMIC_STATE_CULL_MODE_EXT,
            VK_DYNAMIC_STATE_FRONT_FACE_EXT
        },
        .pipelineLayout = {.dsLayouts = {mCameraRenderDataDsLayout}},
        .renderPass = mSsaoResourcesRenderpass,
        .subpassIndex = 0,
        .debugName = "Renderer::mSsaoResourcesPipeline"
    };

    mSsaoResourcesPipeline = {mRenderDevice, specification};
}

void Renderer::createSsaoRenderpass()
{
    VkAttachmentDescription ssaoAttachment {
        .format = mSsaoTexture.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkAttachmentReference ssaoAttachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &ssaoAttachmentRef
    };

    VkSubpassDependency dependency {
        .srcSubpass = 0,
        .dstSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
    };

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &ssaoAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mSsaoRenderpass);
    vulkanCheck(result, "Failed to create renderpass.");
    setRenderpassDebugName(mRenderDevice, mSsaoRenderpass, "Renderer::mSsaoRenderpass");
}

void Renderer::createSsaoFramebuffer()
{
    vkDestroyFramebuffer(mRenderDevice.device, mSsaoFramebuffer, nullptr);

    VkFramebufferCreateInfo framebufferCreateInfo {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = mSsaoRenderpass,
        .attachmentCount = 1,
        .pAttachments = &mSsaoTexture.imageView,
        .width = mWidth,
        .height = mHeight,
        .layers = 1
    };

    VkResult result = vkCreateFramebuffer(mRenderDevice.device, &framebufferCreateInfo, nullptr, &mSsaoFramebuffer);
    vulkanCheck(result, "Failed to create framebuffer.");
    setFramebufferDebugName(mRenderDevice, mSsaoFramebuffer, "Renderer::mSsaoFramebuffer");
}

void Renderer::createSsaoBlurRenderpass()
{
    VkAttachmentDescription ssaoBlurAttachment {
        .format = mSsaoBlurTexture1.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkAttachmentReference ssaoBlurAttachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &ssaoBlurAttachmentRef
    };

    VkSubpassDependency dependency {
        .srcSubpass = 0,
        .dstSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
    };

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &ssaoBlurAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mSsaoBlurRenderpass);
    vulkanCheck(result, "Failed to create renderpass.");
    setRenderpassDebugName(mRenderDevice, mSsaoBlurRenderpass, "Renderer::mSsaoBlurRenderpass");
}

void Renderer::createSsaoBlurFramebuffers()
{
    vkDestroyFramebuffer(mRenderDevice.device, mSsaoBlurFramebuffer1, nullptr);
    vkDestroyFramebuffer(mRenderDevice.device, mSsaoBlurFramebuffer2, nullptr);

    VkFramebufferCreateInfo framebufferCreateInfo {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = mSsaoBlurRenderpass,
        .attachmentCount = 1,
        .pAttachments = &mSsaoBlurTexture1.imageView,
        .width = mWidth,
        .height = mHeight,
        .layers = 1
    };

    VkResult result = vkCreateFramebuffer(mRenderDevice.device, &framebufferCreateInfo, nullptr, &mSsaoBlurFramebuffer1);
    vulkanCheck(result, "Failed to create framebuffer.");
    setFramebufferDebugName(mRenderDevice, mSsaoBlurFramebuffer1, "Renderer::mSsaoBlurFramebuffer1");

    framebufferCreateInfo.pAttachments = &mSsaoBlurTexture2.imageView;
    result = vkCreateFramebuffer(mRenderDevice.device, &framebufferCreateInfo, nullptr, &mSsaoBlurFramebuffer2);
    vulkanCheck(result, "Failed to create framebuffer.");
    setFramebufferDebugName(mRenderDevice, mSsaoBlurFramebuffer2, "Renderer::mSsaoBlurFramebuffer2");
}

void Renderer::createSsaoPipeline()
{
    PipelineSpecification specification {
        .shaderStages = {
            .vertShaderPath = "shaders/fullscreen_render.vert.spv",
            .fragShaderPath = "shaders/ssao.frag.spv"
        },
        .inputAssembly = {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        },
        .tesselation = {
            .patchControlUnits = 0
        },
        .rasterization = {
            .rasterizerDiscardPrimitives = false,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE,
            .lineWidth = 1.f
        },
        .multisampling = {
            .samples = VK_SAMPLE_COUNT_1_BIT
        },
        .depthStencil = {
            .enableDepthTest = VK_FALSE,
            .enableDepthWrite = VK_FALSE,
        },
        .blendStates = {
            {.enable = false}
        },
        .dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        },
        .pipelineLayout = {
            .dsLayouts = {
                mCameraRenderDataDsLayout,
                mSSAODsLayout
            },
            .pushConstantRanges = {
                {
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .offset = 0,
                    .size = sizeof(float) * 7
                }
            }
        },
        .renderPass = mSsaoRenderpass,
        .subpassIndex = 0,
        .debugName = "Renderer::mSsaoPipeline"
    };

    mSsaoPipeline = VulkanGraphicsPipeline(mRenderDevice, specification);
}

void Renderer::createSsaoBlurPipeline()
{
    PipelineSpecification specification {
        .shaderStages = {
            .vertShaderPath = "shaders/fullscreen_render.vert.spv",
            .fragShaderPath = "shaders/ssao_blur.frag.spv"
        },
        .inputAssembly = {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        },
        .tesselation = {
            .patchControlUnits = 0
        },
        .rasterization = {
            .rasterizerDiscardPrimitives = false,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE,
            .lineWidth = 1.f
        },
        .multisampling = {
            .samples = VK_SAMPLE_COUNT_1_BIT
        },
        .depthStencil = {
            .enableDepthTest = VK_FALSE,
            .enableDepthWrite = VK_FALSE,
        },
        .blendStates = {
            {.enable = false}
        },
        .dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        },
        .pipelineLayout = {
            .dsLayouts = {mSingleImageDsLayout},
            .pushConstantRanges = {
                {
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .offset = 0,
                    .size = sizeof(glm::vec2) * 2
                }
            }
        },
        .renderPass = mSsaoBlurRenderpass,
        .subpassIndex = 0,
        .debugName = "Renderer::mSsaoBlurPipeline"
    };

    mSsaoBlurPipeline = VulkanGraphicsPipeline(mRenderDevice, specification);
}

void Renderer::createForwardRenderpass()
{
    VkAttachmentDescription colorAttachment {
        .format = mColorTexture32MS.format,
        .samples = SampleCount,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentDescription depthAttachment {
        .format = mDepthTextureMS.format,
        .samples = SampleCount,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkAttachmentDescription colorResolveAttachment {
        .format = mColorResolve32.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    std::array<VkAttachmentDescription, 3> attachments {
        colorAttachment,
        depthAttachment,
        colorResolveAttachment
    };

    VkAttachmentReference colorAttachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference depthAttachmentRef {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference colorResolveAttachmentRef {
        .attachment = 2,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pResolveAttachments = &colorResolveAttachmentRef,
        .pDepthStencilAttachment = &depthAttachmentRef,
    };

    VkSubpassDependency dependency {
        .srcSubpass = 0,
        .dstSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT |
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                             VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
                             VK_ACCESS_SHADER_READ_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
    };

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mForwardRenderpass);
    vulkanCheck(result, "Failed to create renderpass.");
    setRenderpassDebugName(mRenderDevice, mForwardRenderpass, "Renderer::mForwardRenderpass");
}

void Renderer::createForwardFramebuffer()
{
    vkDestroyFramebuffer(mRenderDevice.device, mForwardPassFramebuffer, nullptr);

    std::array<VkImageView, 3> attachments {
        mColorTexture32MS.imageView,
        mDepthTextureMS.imageView,
        mColorResolve32.imageView
    };

    VkFramebufferCreateInfo framebufferCreateInfo {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = mForwardRenderpass,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = mWidth,
        .height = mHeight,
        .layers = 1
    };

    VkResult result = vkCreateFramebuffer(mRenderDevice.device, &framebufferCreateInfo, nullptr, &mForwardPassFramebuffer);
    vulkanCheck(result, "Failed to create framebuffer.");
    setFramebufferDebugName(mRenderDevice, mForwardPassFramebuffer, "Renderer::mForwardPassFramebuffer");
}

void Renderer::createOpaqueForwardPassPipeline()
{
    PipelineSpecification specification {
        .shaderStages = {
            .vertShaderPath = "shaders/mesh.vert.spv",
            .fragShaderPath = "shaders/forward_pass.frag.spv"
        },
        .vertexInput = {
            .bindings = InstancedMesh::bindingDescriptionsInstanced(),
            .attributes = InstancedMesh::attributeDescriptionsInstanced()
        },
        .inputAssembly = {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        },
        .tesselation = {
            .patchControlUnits = 0
        },
        .rasterization = {
            .rasterizerDiscardPrimitives = false,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .lineWidth = 1.f
        },
        .multisampling = {
            .samples = SampleCount
        },
        .depthStencil = {
            .enableDepthTest = VK_TRUE,
            .enableDepthWrite = VK_FALSE,
            .depthCompareOp = VK_COMPARE_OP_EQUAL
        },
        .blendStates = {
            {.enable = false}
        },
        .dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
            VK_DYNAMIC_STATE_CULL_MODE_EXT,
            VK_DYNAMIC_STATE_FRONT_FACE_EXT
        },
        .pipelineLayout = {
            .dsLayouts = {
                mCameraRenderDataDsLayout,
                mForwardShadingDsLayout,
                mLightsDsLayout,
                mMaterialsDsLayout,
            },
            .pushConstantRanges =  {
                {
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .offset = 0,
                    .size = sizeof(glm::mat4)
                }
            }
        },
        .renderPass = mForwardRenderpass,
        .subpassIndex = 0,
        .debugName = "Renderer::mOpaqueForwardPassPipeline"
    };

    mOpaqueForwardPassPipeline = VulkanGraphicsPipeline(mRenderDevice, specification);
}

void Renderer::createTransparentForwardPassPipeline()
{
    PipelineSpecification specification {
        .shaderStages = {
            .vertShaderPath = "shaders/transparent_mesh.vert.spv",
            .fragShaderPath = "shaders/forward_pass.frag.spv"
        },
        .vertexInput = {
            .bindings = InstancedMesh::bindingDescriptions(),
            .attributes = InstancedMesh::attributeDescriptions()
        },
        .inputAssembly = {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        },
        .tesselation = {
            .patchControlUnits = 0
        },
        .rasterization = {
            .rasterizerDiscardPrimitives = false,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .lineWidth = 1.f
        },
        .multisampling = {
            .samples = SampleCount
        },
        .depthStencil = {
            .enableDepthTest = VK_TRUE,
            .enableDepthWrite = VK_FALSE,
            .depthCompareOp = VK_COMPARE_OP_LESS
        },
        .blendStates = {
            {
                .enable = true,
                .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .alphaBlendOp = VK_BLEND_OP_ADD
            }
        },
        .dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
            VK_DYNAMIC_STATE_CULL_MODE_EXT,
            VK_DYNAMIC_STATE_FRONT_FACE_EXT
        },
        .pipelineLayout = {
            .dsLayouts = {
                mCameraRenderDataDsLayout,
                mForwardShadingDsLayout,
                mLightsDsLayout,
                mMaterialsDsLayout,
            },
            .pushConstantRanges =  {
                {
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .offset = 0,
                    .size = sizeof(glm::mat4)
                },
                {
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                    .offset = sizeof(glm::mat4),
                    .size = sizeof(glm::mat4)
                }
            }
        },
        .renderPass = mForwardRenderpass,
        .subpassIndex = 0,
        .debugName = "Renderer::mTransparentForwardPassPipeline"
    };

    mTransparentForwardPassPipeline = VulkanGraphicsPipeline(mRenderDevice, specification);
}

void Renderer::createPostProcessingRenderpass()
{
    VkAttachmentDescription colorAttachment {
        .format = mColorTexture8U.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference colorAttachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef
    };

    VkSubpassDependency dependency {
        .srcSubpass = 0,
        .dstSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT ,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
    };

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mPostProcessingRenderpass);
    vulkanCheck(result, "Failed to create renderpass.");
    setRenderpassDebugName(mRenderDevice, mPostProcessingRenderpass, "Renderer::mPostProcessingRenderpass");
}

void Renderer::createPostProcessingFramebuffer()
{
    vkDestroyFramebuffer(mRenderDevice.device, mPostProcessingFramebuffer, nullptr);

    VkFramebufferCreateInfo framebufferCreateInfo {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = mPostProcessingRenderpass,
        .attachmentCount = 1,
        .pAttachments = &mColorTexture8U.imageView,
        .width = mWidth,
        .height = mHeight,
        .layers = 1
    };

    VkResult result = vkCreateFramebuffer(mRenderDevice.device, &framebufferCreateInfo, nullptr, &mPostProcessingFramebuffer);
    vulkanCheck(result, "Failed to create framebuffer.");
    setFramebufferDebugName(mRenderDevice, mPostProcessingFramebuffer, "Renderer::mPostProcessingFramebuffer");
}

void Renderer::createPostProcessingPipeline()
{
    PipelineSpecification specification {
        .shaderStages = {
            .vertShaderPath = "shaders/fullscreen_render.vert.spv",
            .fragShaderPath = "shaders/post_process.frag.spv"
        },
        .inputAssembly = {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        },
        .tesselation = {
            .patchControlUnits = 0
        },
        .rasterization = {
            .rasterizerDiscardPrimitives = false,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE,
            .lineWidth = 1.f
        },
        .multisampling = {
            .samples = VK_SAMPLE_COUNT_1_BIT
        },
        .depthStencil = {
            .enableDepthTest = VK_FALSE,
            .enableDepthWrite = VK_FALSE,
        },
        .blendStates = {
            {.enable = false}
        },
        .dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        },
        .pipelineLayout = {
            .dsLayouts = {mPostProcessingDsLayout},
            .pushConstantRanges = {
                {
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .offset = 0,
                    .size = sizeof(uint32_t) * 5
                }
            }
        },
        .renderPass = mPostProcessingRenderpass,
        .subpassIndex = 0,
        .debugName = "Renderer::mPostProcessingPipeline"
    };

    mPostProcessingPipeline = VulkanGraphicsPipeline(mRenderDevice, specification);
}

void Renderer::createWireframeRenderpass()
{
    VkAttachmentDescription colorAttachment {
        .format = mColorTexture8U.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentDescription depthAttachment {
        .format = mDepthTexture.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
    };

    std::array<VkAttachmentDescription, 2> attachments {
        colorAttachment,
        depthAttachment
    };

    VkAttachmentReference colorAttachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference depthAttachmentRef {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pDepthStencilAttachment = &depthAttachmentRef
    };

    VkSubpassDependency dependency {
        .srcSubpass = 0,
        .dstSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
            VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
    };

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mWireframeRenderpass);
    vulkanCheck(result, "Failed to create renderpass.");
    setRenderpassDebugName(mRenderDevice, mWireframeRenderpass, "Renderer::mWireframeRenderpass");
}

void Renderer::createWireframeFramebuffer()
{
    vkDestroyFramebuffer(mRenderDevice.device, mWireframeFramebuffer, nullptr);

    std::array<VkImageView, 2> attachments {
        mColorTexture8U.imageView,
        mDepthTexture.imageView
    };

    VkFramebufferCreateInfo framebufferCreateInfo {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = mWireframeRenderpass,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = mWidth,
        .height = mHeight,
        .layers = 1
    };

    VkResult result = vkCreateFramebuffer(mRenderDevice.device, &framebufferCreateInfo, nullptr, &mWireframeFramebuffer);
    vulkanCheck(result, "Failed to create framebuffer.");
    setFramebufferDebugName(mRenderDevice, mWireframeFramebuffer, "Renderer::mWireframeFramebuffer");
}

void Renderer::createWireframePipeline()
{
    PipelineSpecification specification {
        .shaderStages = {
            .vertShaderPath = "shaders/wireframe.vert.spv",
            .geomShaderPath = "shaders/wireframe.geom.spv",
            .fragShaderPath = "shaders/wireframe.frag.spv"
        },
        .vertexInput {
            .bindings = InstancedMesh::bindingDescriptionsInstanced(),
            .attributes = InstancedMesh::attributeDescriptionsInstanced()
        },
        .inputAssembly = {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        },
        .rasterization = {
            .polygonMode = VK_POLYGON_MODE_FILL,
            .lineWidth = 1.f
        },
        .multisampling = {
            .samples = VK_SAMPLE_COUNT_1_BIT
        },
        .depthStencil = {
            .enableDepthTest = true,
            .enableDepthWrite = false,
            .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL
        },
        .blendStates = {
            {
                .enable = true,
                .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .alphaBlendOp = VK_BLEND_OP_ADD
            }
        },
        .dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR,
            VK_DYNAMIC_STATE_CULL_MODE_EXT,
            VK_DYNAMIC_STATE_FRONT_FACE_EXT
        },
        .pipelineLayout = {
            .dsLayouts = {mCameraRenderDataDsLayout},
            .pushConstantRanges = {
                {
                    .stageFlags = VK_SHADER_STAGE_GEOMETRY_BIT,
                    .offset = 0,
                    .size = sizeof(glm::mat4)
                },
                {
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .offset = sizeof(glm::mat4),
                    .size = sizeof(float) * 5
                }
            }
        },
        .renderPass = mWireframeRenderpass,
        .subpassIndex = 0,
        .debugName = "Renderer::mWireframePipeline"
    };

    mWireframePipeline = {mRenderDevice, specification};
}

void Renderer::createGridRenderpass()
{
    VkAttachmentDescription colorAttachment {
        .format = mColorTexture8U.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentDescription depthAttachment {
        .format = mDepthTexture.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL
    };

    std::array<VkAttachmentDescription, 2> attachments {{
        colorAttachment,
        depthAttachment
    }};

    VkAttachmentReference colorAttachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference depthAttachmentRef {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pDepthStencilAttachment = &depthAttachmentRef
    };

    VkSubpassDependency dependency {
        .srcSubpass = 0,
        .dstSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                         VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
    };

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mGridRenderpass);
    vulkanCheck(result, "Failed to create renderpass.");
    setRenderpassDebugName(mRenderDevice, mGridRenderpass, "Renderer::mGridRenderpass");
}

void Renderer::createGridFramebuffer()
{
    vkDestroyFramebuffer(mRenderDevice.device, mGridFramebuffer, nullptr);

    std::array<VkImageView, 2> imageViews {
        mColorTexture8U.imageView,
        mDepthTexture.imageView,
    };

    VkFramebufferCreateInfo framebufferCreateInfo {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = mGridRenderpass,
        .attachmentCount = static_cast<uint32_t>(imageViews.size()),
        .pAttachments = imageViews.data(),
        .width = mWidth,
        .height = mHeight,
        .layers = 1
    };

    VkResult result = vkCreateFramebuffer(mRenderDevice.device, &framebufferCreateInfo, nullptr, &mGridFramebuffer);
    vulkanCheck(result, "Failed to create framebuffer.");
    setFramebufferDebugName(mRenderDevice, mGridFramebuffer, "Renderer::mGridFramebuffer");
}

void Renderer::createGridPipeline()
{
    PipelineSpecification specification {
        .shaderStages = {
            .vertShaderPath = "shaders/grid.vert.spv",
            .fragShaderPath = "shaders/grid.frag.spv"
        },
        .inputAssembly = {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        },
        .tesselation = {
            .patchControlUnits = 0
        },
        .rasterization = {
            .rasterizerDiscardPrimitives = false,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE,
            .lineWidth = 1.f
        },
        .multisampling = {
            .samples = VK_SAMPLE_COUNT_1_BIT
        },
        .depthStencil = {
            .enableDepthTest = VK_TRUE,
            .enableDepthWrite = VK_FALSE,
            .depthCompareOp = VK_COMPARE_OP_LESS
        },
        .blendStates = {
            {
                .enable = true,
                .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .alphaBlendOp = VK_BLEND_OP_ADD
            }
        },
        .dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        },
        .pipelineLayout = {
            .dsLayouts = {
                mCameraRenderDataDsLayout,
            },
            .pushConstantRanges = {
                {
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .offset = 0,
                    .size = sizeof(GridData)
                }
            }
        },
        .renderPass = mGridRenderpass,
        .subpassIndex = 0,
        .debugName = "Renderer::mGridPipeline"
    };

    mGridPipeline = VulkanGraphicsPipeline(mRenderDevice, specification);
}

void Renderer::createLightBuffers()
{
    mDirLightSSBO = {mRenderDevice, MaxDirLights * sizeof(DirectionalLight), BufferType::Storage, MemoryType::Device};
    mSpotLightSSBO = {mRenderDevice, MaxSpotLights * sizeof(SpotLight), BufferType::Storage, MemoryType::Device};
    mPointLightSSBO = {mRenderDevice, MaxPointLights * sizeof(PointLight), BufferType::Storage, MemoryType::Device};

    mDirLightSSBO.setDebugName("Renderer::mDirLightSSBO");
    mSpotLightSSBO.setDebugName("Renderer::mSpotLightSSBO");
    mPointLightSSBO.setDebugName("Renderer::mPointLightSSBO");
}

void Renderer::createLightIconTextures()
{
    stbi_set_flip_vertically_on_load(true);
    LoadedImage dirLightIcon("assets/textures/lights/directional3.png");
    LoadedImage pointLightIcon("assets/textures/lights/lightbulb4.png");
    LoadedImage spotLightIcon("assets/textures/lights/spotlight2.png");
    stbi_set_flip_vertically_on_load(false);

    assert(dirLightIcon.success());
    assert(pointLightIcon.success());
    assert(spotLightIcon.success());

    uint32_t iconWidth = 512;
    uint32_t iconHeight = 512;

    TextureSpecification specification {
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .width = iconWidth,
        .height = iconHeight,
        .layerCount = 1,
        .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
        .imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                      VK_IMAGE_USAGE_SAMPLED_BIT,
        .imageAspect = VK_IMAGE_ASPECT_COLOR_BIT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .magFilter = TextureMagFilter::Linear,
        .minFilter = TextureMinFilter::Linear,
        .wrapS = TextureWrap::ClampToEdge,
        .wrapT = TextureWrap::ClampToEdge,
        .generateMipMaps = true
    };

    mDirLightIcon = {mRenderDevice, specification, dirLightIcon.data()};
    mPointLightIcon = {mRenderDevice, specification, pointLightIcon.data()};
    mSpotLightIcon = {mRenderDevice, specification, spotLightIcon.data()};

    mDirLightIcon.setDebugName("Renderer::mDirLightIcon");
    mPointLightIcon.setDebugName("Renderer::mPointLightIcon");
    mSpotLightIcon.setDebugName("Renderer::mSpotLightIcon");
}

void Renderer::createLightIconTextureDsLayout()
{
    DsLayoutSpecification specification {
        .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR,
        .bindings = {
            binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
        },
        .debugName = "Renderer::mIconTextureDsLayout"
    };

    mIconTextureDsLayout = {mRenderDevice, specification};
}

void Renderer::createLightIconRenderpass()
{
    VkAttachmentDescription colorAttachment {
        .format = mColorTexture8U.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkAttachmentReference colorAttachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef
    };

    VkSubpassDependency dependency {
        .srcSubpass = 0,
        .dstSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
                         VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
    };

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mLightIconRenderpass);
    vulkanCheck(result, "Failed to create renderpass.");
    setRenderpassDebugName(mRenderDevice, mLightIconRenderpass, "Renderer::mLightIconRenderpass");
}

void Renderer::createLightIconFramebuffer()
{
    vkDestroyFramebuffer(mRenderDevice.device, mLightIconFramebuffer, nullptr);

    VkFramebufferCreateInfo framebufferCreateInfo {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = mLightIconRenderpass,
        .attachmentCount = 1,
        .pAttachments = &mColorTexture8U.imageView,
        .width = mWidth,
        .height = mHeight,
        .layers = 1
    };

    VkResult result = vkCreateFramebuffer(mRenderDevice.device, &framebufferCreateInfo, nullptr, &mLightIconFramebuffer);
    vulkanCheck(result, "Failed to create framebuffer.");
    setFramebufferDebugName(mRenderDevice, mLightIconFramebuffer, "Renderer::mLightIconFramebuffer");
}

void Renderer::createLightIconPipeline()
{
    PipelineSpecification specification {
        .shaderStages = {
            .vertShaderPath = "shaders/light_icon.vert.spv",
            .fragShaderPath = "shaders/light_icon.frag.spv"
        },
        .inputAssembly = {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        },
        .rasterization = {
            .rasterizerDiscardPrimitives = false,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE,
            .lineWidth = 1.f
        },
        .multisampling = {
            .samples = VK_SAMPLE_COUNT_1_BIT
        },
        .depthStencil = {
            .enableDepthTest = VK_FALSE,
            .enableDepthWrite = VK_FALSE,
        },
        .blendStates = {
            {
                .enable = true,
                .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .alphaBlendOp = VK_BLEND_OP_ADD
            }
        },
        .dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        },
        .pipelineLayout = {
            .dsLayouts = {
                mCameraRenderDataDsLayout,
                mIconTextureDsLayout
            },
            .pushConstantRanges = {
                {
                    .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                    .offset = 0,
                    .size = sizeof(glm::vec3)
                }
            }
        },
        .renderPass = mLightIconRenderpass,
        .subpassIndex = 0,
        .debugName = "Renderer::mLightIconPipeline"
    };

    mLightIconPipeline = VulkanGraphicsPipeline(mRenderDevice, specification);
}

void Renderer::createGizmoIconResources()
{
    LoadedImage translationIcon("assets/textures/gizmo/move.png");
    LoadedImage rotationIcon("assets/textures/gizmo/rotate.png");
    LoadedImage scaleIcon("assets/textures/gizmo/scale.png");
    LoadedImage globalIcon("assets/textures/gizmo/global.png");
    LoadedImage localIcon("assets/textures/gizmo/local.png");

    assert(translationIcon.success());
    assert(rotationIcon.success());
    assert(scaleIcon.success());
    assert(globalIcon.success());
    assert(localIcon.success());

    uint32_t iconWidth = 32;
    uint32_t iconHeight = 32;

    TextureSpecification specification {
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .width = iconWidth,
        .height = iconHeight,
        .layerCount = 1,
        .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
        .imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                      VK_IMAGE_USAGE_SAMPLED_BIT,
        .imageAspect = VK_IMAGE_ASPECT_COLOR_BIT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .magFilter = TextureMagFilter::Nearest,
        .minFilter = TextureMinFilter::Nearest,
        .wrapS = TextureWrap::ClampToEdge,
        .wrapT = TextureWrap::ClampToEdge,
        .generateMipMaps = false
    };

    mTranslateIcon = {mRenderDevice, specification, translationIcon.data()};
    mRotateIcon = {mRenderDevice, specification, rotationIcon.data()};
    mScaleIcon = {mRenderDevice, specification, scaleIcon.data()};
    mGlobalIcon = {mRenderDevice, specification, globalIcon.data()};
    mLocalIcon = {mRenderDevice, specification, localIcon.data()};

    mTranslateIcon.setDebugName("Renderer::mTranslateIcon");
    mRotateIcon.setDebugName("Renderer::mRotateIcon");
    mScaleIcon.setDebugName("Renderer::mScaleIcon");
    mGlobalIcon.setDebugName("Renderer::mGlobalIcon");
    mLocalIcon.setDebugName("Renderer::mLocalIcon");

    createSingleImageDs(mTranslateIconDs, mTranslateIcon, "Renderer::mTranslateIconDs");
    createSingleImageDs(mRotateIconDs, mRotateIcon, "Renderer::mRotateIconDs");
    createSingleImageDs(mScaleIconDs, mScaleIcon, "Renderer::mScaleIconDs");
    createSingleImageDs(mGlobalIconDs, mGlobalIcon, "Renderer::mGlobalIconDs");
    createSingleImageDs(mLocalIconDs, mLocalIcon, "Renderer::mLocalIconDs");
}

void Renderer::createIrradianceMap()
{
    uint32_t irradianceMapSize = 32;

    TextureSpecification specification {
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .width = irradianceMapSize,
        .height = irradianceMapSize,
        .layerCount = 6,
        .imageViewType = VK_IMAGE_VIEW_TYPE_CUBE,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                      VK_IMAGE_USAGE_SAMPLED_BIT,
        .imageAspect = VK_IMAGE_ASPECT_COLOR_BIT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .magFilter = TextureMagFilter::Linear,
        .minFilter = TextureMinFilter::LinearMipmapNearest,
        .wrapS = TextureWrap::ClampToEdge,
        .wrapT = TextureWrap::ClampToEdge,
        .wrapR = TextureWrap::ClampToEdge,
        .generateMipMaps = false,
        .createFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
    };

    mIrradianceMap = VulkanTexture(mRenderDevice, specification);
    mIrradianceMap.setDebugName("Renderer::mIrradianceMap");
}

void Renderer::createPrefilterMap()
{
    uint32_t prefilterMapSize = 1024;

    TextureSpecification specification {
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .width = prefilterMapSize,
        .height = prefilterMapSize,
        .layerCount = 6,
        .imageViewType = VK_IMAGE_VIEW_TYPE_CUBE,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                      VK_IMAGE_USAGE_SAMPLED_BIT,
        .imageAspect = VK_IMAGE_ASPECT_COLOR_BIT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .magFilter = TextureMagFilter::Linear,
        .minFilter = TextureMinFilter::LinearMipmapLinear,
        .wrapS = TextureWrap::ClampToEdge,
        .wrapT = TextureWrap::ClampToEdge,
        .wrapR = TextureWrap::ClampToEdge,
        .generateMipMaps = true,
        .createFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
    };

    mPrefilterMap = VulkanTexture(mRenderDevice, specification);
    mPrefilterMap.createMipLevelImageViews(VK_IMAGE_VIEW_TYPE_CUBE);
    mPrefilterMap.setDebugName("Renderer::mPrefilterMap");
}

void Renderer::createBrdfLut()
{
    TextureSpecification specification {
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .width = 512,
        .height = 512,
        .layerCount = 1,
        .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                      VK_IMAGE_USAGE_SAMPLED_BIT,
        .imageAspect = VK_IMAGE_ASPECT_COLOR_BIT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .magFilter = TextureMagFilter::Linear,
        .minFilter = TextureMinFilter::LinearMipmapNearest,
        .wrapS = TextureWrap::ClampToEdge,
        .wrapT = TextureWrap::ClampToEdge,
        .wrapR = TextureWrap::ClampToEdge,
        .generateMipMaps = false,
    };

    mBrdfLut = VulkanTexture(mRenderDevice, specification);
    mBrdfLut.setDebugName("Renderer::mBrdfLut");
}

void Renderer::createEnvMapViewsUBO()
{
    static const std::array<glm::mat4, 6> views {
        glm::lookAt(glm::vec3(0.f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAt(glm::vec3(0.f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
    };

    mEnvMapViewsUBO = VulkanBuffer(mRenderDevice,
                                   sizeof(views),
                                   BufferType::Uniform,
                                   MemoryType::Device,
                                   views.data());
    mEnvMapViewsUBO.setDebugName("Renderer::mEnvMapViewsUBO");
}

void Renderer::createIrradianceViewsUBO()
{
    static const std::array<glm::mat4, 6> views {
        glm::lookAt(glm::vec3(0.f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        glm::lookAt(glm::vec3(0.f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
        glm::lookAt(glm::vec3(0.f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
    };

    mIrradianceViewsUBO = VulkanBuffer(mRenderDevice,
                                       sizeof(views),
                                       BufferType::Uniform,
                                       MemoryType::Device,
                                       views.data());
    mIrradianceViewsUBO.setDebugName("Renderer::mIrradianceViewsUBO");
}

void Renderer::createCubemapConvertRenderpass()
{
    VkAttachmentDescription attachment {
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
    };

    VkAttachmentReference attachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentRef,
    };

    VkSubpassDependency dependency {
        .srcSubpass = 0,
        .dstSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
    };

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mCubemapConvertRenderpass);
    vulkanCheck(result, "Failed to create renderpass.");
    setRenderpassDebugName(mRenderDevice, mCubemapConvertRenderpass, "Renderer::mCubemapConvertRenderpass");
}

void Renderer::createIrradianceConvolutionRenderpass()
{
    VkAttachmentDescription attachment {
        .format = mIrradianceMap.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkAttachmentReference attachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentRef,
    };

    VkSubpassDependency dependency {
        .srcSubpass = 0,
        .dstSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
    };

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mIrradianceConvolutionRenderpass);
    vulkanCheck(result, "Failed to create renderpass.");
    setRenderpassDebugName(mRenderDevice, mIrradianceConvolutionRenderpass, "Renderer::mIrradianceConvolutionRenderpass");
}

void Renderer::createPrefilterRenderpass()
{
    VkAttachmentDescription attachment {
        .format = mPrefilterMap.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkAttachmentReference attachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentRef,
    };

    VkSubpassDependency dependency {
        .srcSubpass = 0,
        .dstSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
    };

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mPrefilterRenderpass);
    vulkanCheck(result, "Failed to create renderpass.");
    setRenderpassDebugName(mRenderDevice, mPrefilterRenderpass, "Renderer::mPrefilterRenderpass");
}

void Renderer::createBrdfLutRenderpass()
{
    VkAttachmentDescription attachment {
        .format = mBrdfLut.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkAttachmentReference attachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentRef,
    };

    VkSubpassDependency dependency {
        .srcSubpass = 0,
        .dstSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
    };

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mBrdfLutRenderpass);
    vulkanCheck(result, "Failed to create renderpass.");
    setRenderpassDebugName(mRenderDevice, mBrdfLutRenderpass, "Renderer::mBrdfLutRenderpass");
}

void Renderer::createCubemapConvertDsLayout()
{
    DsLayoutSpecification dsLayoutSpecification {
        .bindings = {
            binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_GEOMETRY_BIT),
            binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
        },
        .debugName = "Renderer::mCubemapConvertDsLayout"
    };

    mCubemapConvertDsLayout = VulkanDsLayout(mRenderDevice, dsLayoutSpecification);
}

void Renderer::createIrradianceConvolutionDsLayout()
{
    DsLayoutSpecification specification {
        .bindings = {
            binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_GEOMETRY_BIT),
            binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
        },
        .debugName = "mIrradianceConvolutionDsLayout::mDsLayout"
    };

    mIrradianceConvolutionDsLayout = {mRenderDevice, specification};
}

void Renderer::createCubemapConvertPipeline()
{
    PipelineSpecification specification {
        .shaderStages = {
            .vertShaderPath = "shaders/quad.vert.spv",
            .geomShaderPath = "shaders/equirectangular.geom.spv",
            .fragShaderPath = "shaders/equirectangular.frag.spv"
        },
        .inputAssembly = {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        },
        .rasterization = {
            .rasterizerDiscardPrimitives = false,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .lineWidth = 1.f
        },
        .multisampling = {
            .samples = VK_SAMPLE_COUNT_1_BIT
        },
        .depthStencil = {
            .enableDepthTest = false,
            .enableDepthWrite = false
        },
        .blendStates = {
            {.enable = false}
        },
        .dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        },
        .pipelineLayout = {.dsLayouts = {mCubemapConvertDsLayout}},
        .renderPass = mCubemapConvertRenderpass,
        .subpassIndex = 0,
        .debugName = "Renderer::mCubemapConvertPipeline"
    };

    mCubemapConvertPipeline = VulkanGraphicsPipeline(mRenderDevice, specification);
}

void Renderer::createConvolutionPipeline()
{
    PipelineSpecification specification {
        .shaderStages = {
            .vertShaderPath = "shaders/quad.vert.spv",
            .geomShaderPath = "shaders/equirectangular.geom.spv",
            .fragShaderPath = "shaders/irradiance_convolution.frag.spv"
        },
        .inputAssembly = {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        },
        .viewportState = {
            .viewport = VkViewport {
                .x = 0.f,
                .y = static_cast<float>(mIrradianceMap.height),
                .width = static_cast<float>(mIrradianceMap.width),
                .height = -static_cast<float>(mIrradianceMap.height),
                .minDepth = 0.f,
                .maxDepth = 1.f
            },
            .scissor = VkRect2D {
                .offset = {.x = 0, .y = 0},
                .extent = {
                    .width = static_cast<uint32_t>(mIrradianceMap.width),
                    .height = static_cast<uint32_t>(mIrradianceMap.height)
                }
            }
        },
        .rasterization = {
            .rasterizerDiscardPrimitives = false,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .lineWidth = 1.f
        },
        .multisampling = {
            .samples = VK_SAMPLE_COUNT_1_BIT
        },
        .depthStencil = {
            .enableDepthTest = false,
            .enableDepthWrite = false
        },
        .blendStates = {
            {.enable = false}
        },
        .pipelineLayout = {.dsLayouts = {mIrradianceConvolutionDsLayout}},
        .renderPass = mIrradianceConvolutionRenderpass,
        .subpassIndex = 0,
        .debugName = "Renderer::mIrradianceConvolutionPipeline"
    };

    mIrradianceConvolutionPipeline = VulkanGraphicsPipeline(mRenderDevice, specification);
}

void Renderer::createPrefilterPipeline()
{
    PipelineSpecification specification {
        .shaderStages = {
            .vertShaderPath = "shaders/quad.vert.spv",
            .geomShaderPath = "shaders/equirectangular.geom.spv",
            .fragShaderPath = "shaders/prefilter.frag.spv"
        },
        .inputAssembly = {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        },
        .rasterization = {
            .rasterizerDiscardPrimitives = false,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .lineWidth = 1.f
        },
        .multisampling = {
            .samples = VK_SAMPLE_COUNT_1_BIT
        },
        .depthStencil = {
            .enableDepthTest = false,
            .enableDepthWrite = false
        },
        .blendStates = {
            {.enable = false}
        },
        .dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        },
        .pipelineLayout = {
            .dsLayouts = {
                mIrradianceConvolutionDsLayout
            },
            .pushConstantRanges = {
                {
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .offset = 0,
                    .size = sizeof(float)
                }
            }
        },
        .renderPass = mPrefilterRenderpass,
        .subpassIndex = 0,
        .debugName = "Renderer::mPrefilterPipeline"
    };

    mPrefilterPipeline = VulkanGraphicsPipeline(mRenderDevice, specification);
}

void Renderer::createBrdfLutPipeline()
{
    PipelineSpecification specification {
        .shaderStages = {
            .vertShaderPath = "shaders/fullscreen_render.vert.spv",
            .fragShaderPath = "shaders/integrate_brdf.frag.spv"
        },
        .inputAssembly = {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        },
        .viewportState = {
            .viewport = VkViewport {
                .x = 0,
                .y = 0,
                .width = static_cast<float>(mBrdfLut.width),
                .height = static_cast<float>(mBrdfLut.height),
                .minDepth = 0.0,
                .maxDepth = 1.0
            },
            .scissor = VkRect2D {
                .offset = {.x = 0, .y = 0},
                .extent = {
                    .width = mBrdfLut.width,
                    .height = mBrdfLut.height
                }
            }
        },
        .rasterization = {
            .rasterizerDiscardPrimitives = false,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .lineWidth = 1.f
        },
        .multisampling = {
            .samples = VK_SAMPLE_COUNT_1_BIT
        },
        .depthStencil = {
            .enableDepthTest = false,
            .enableDepthWrite = false
        },
        .blendStates = {
            {.enable = false}
        },
        .renderPass = mBrdfLutRenderpass,
        .subpassIndex = 0,
        .debugName = "Renderer::mBrdfLutPipeline"
    };

    mBrdfLutPipeline = {mRenderDevice, specification};
}

void Renderer::createIrradianceConvolutionFramebuffer()
{
    VkFramebufferCreateInfo framebufferCreateInfo {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = mIrradianceConvolutionRenderpass,
        .attachmentCount = 1,
        .pAttachments = &mIrradianceMap.imageView,
        .width = mIrradianceMap.width,
        .height = mIrradianceMap.height,
        .layers = mIrradianceMap.layerCount
    };

    VkResult result = vkCreateFramebuffer(mRenderDevice.device, &framebufferCreateInfo, nullptr, &mIrradianceConvolutionFramebuffer);
    vulkanCheck(result, "Failed to create framebuffer.");
    setFramebufferDebugName(mRenderDevice, mIrradianceConvolutionFramebuffer, "Renderer::mIrradianceConvolutionFramebuffer");
}

void Renderer::createPrefilterFramebuffers()
{
    uint32_t width = mPrefilterMap.width;
    uint32_t height = mPrefilterMap.height;

    mPrefilterFramebuffers.resize(mPrefilterMap.mipLevels);
    for (uint32_t i = 0; i < mPrefilterMap.mipLevels; ++i)
    {
        VkFramebufferCreateInfo framebufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = mPrefilterRenderpass,
            .attachmentCount = 1,
            .pAttachments = &mPrefilterMap.mipLevelImageViews.at(i),
            .width = width,
            .height = height,
            .layers = mPrefilterMap.layerCount
        };

        VkResult result = vkCreateFramebuffer(mRenderDevice.device, &framebufferCreateInfo, nullptr, &mPrefilterFramebuffers.at(i));
        vulkanCheck(result, "Failed to create framebuffer.");
        setFramebufferDebugName(mRenderDevice, mPrefilterFramebuffers.at(i), "Renderer::mPrefilterFramebuffers");

        width /= 2;
        height /= 2;
    }
}

void Renderer::createBrdfLutFramebuffer()
{
    VkFramebufferCreateInfo framebufferCreateInfo {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = mBrdfLutRenderpass,
        .attachmentCount = 1,
        .pAttachments = &mBrdfLut.imageView,
        .width = mBrdfLut.width,
        .height = mBrdfLut.height,
        .layers = 1
    };

    VkResult result = vkCreateFramebuffer(mRenderDevice.device, &framebufferCreateInfo, nullptr, &mBrdfLutFramebuffer);
    vulkanCheck(result, "Failed to create framebuffer.");
    setFramebufferDebugName(mRenderDevice, mBrdfLutFramebuffer, "Renderer::mBrdfLutFramebuffer");
}

void Renderer::createEquirectangularTexture(const std::string& path)
{
    int32_t width;
    int32_t height;

    float* data = stbi_loadf(path.data(), &width, &height, nullptr, 4);

    assert(data);

    TextureSpecification specification {
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .width = static_cast<uint32_t>(width),
        .height = static_cast<uint32_t>(height),
        .layerCount = 1,
        .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
        .imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                      VK_IMAGE_USAGE_SAMPLED_BIT,
        .imageAspect = VK_IMAGE_ASPECT_COLOR_BIT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .magFilter = TextureMagFilter::Linear,
        .minFilter = TextureMinFilter::LinearMipmapNearest,
        .wrapS = TextureWrap::ClampToEdge,
        .wrapT = TextureWrap::ClampToEdge,
        .generateMipMaps = false
    };

    mEquirectangularTexture = VulkanTexture(mRenderDevice, specification, data);
    mEquirectangularTexture.setDebugName("Renderer::mEquirectangularTexture");

    stbi_image_free(data);
}

void Renderer::createEnvMap()
{
    mEnvMapFaceSize = mEquirectangularTexture.height;

    TextureSpecification specification {
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .width = static_cast<uint32_t>(mEnvMapFaceSize),
        .height = static_cast<uint32_t>(mEnvMapFaceSize),
        .layerCount = 6,
        .imageViewType = VK_IMAGE_VIEW_TYPE_CUBE,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                      VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                      VK_IMAGE_USAGE_SAMPLED_BIT,
        .imageAspect = VK_IMAGE_ASPECT_COLOR_BIT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .magFilter = TextureMagFilter::Linear,
        .minFilter = TextureMinFilter::Linear,
        .wrapS = TextureWrap::ClampToEdge,
        .wrapT = TextureWrap::ClampToEdge,
        .wrapR = TextureWrap::ClampToEdge,
        .generateMipMaps = true,
        .createFlags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
    };

    mEnvMap = VulkanTexture(mRenderDevice, specification);
    mEnvMap.createMipLevelImageViews(VK_IMAGE_VIEW_TYPE_CUBE);
    mEnvMap.setDebugName("Renderer::mEnvMap");
}

void Renderer::createCubemapConvertFramebuffer()
{
    vkDestroyFramebuffer(mRenderDevice.device, mCubemapConvertFramebuffer, nullptr);

    VkFramebufferCreateInfo framebufferCreateInfo {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = mCubemapConvertRenderpass,
        .attachmentCount = 1,
        .pAttachments = &mEnvMap.mipLevelImageViews.at(0),
        .width = mEnvMap.width,
        .height = mEnvMap.height,
        .layers = 6
    };

    VkResult result = vkCreateFramebuffer(mRenderDevice.device, &framebufferCreateInfo, nullptr, &mCubemapConvertFramebuffer);
    vulkanCheck(result, "Failed to create framebuffer.");
    setFramebufferDebugName(mRenderDevice, mCubemapConvertFramebuffer, "Renderer::mCubemapConvertFramebuffer");
}

void Renderer::createCubemapConvertDs()
{
    vkFreeDescriptorSets(mRenderDevice.device, mRenderDevice.descriptorPool, 1, &mCubemapConvertDs);

    VkDescriptorSetLayout dsLayout = mCubemapConvertDsLayout;
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = mRenderDevice.descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &dsLayout
    };

    VkResult result = vkAllocateDescriptorSets(mRenderDevice.device, &descriptorSetAllocateInfo, &mCubemapConvertDs);
    vulkanCheck(result, "Failed to allocate descriptor set.");

    VkDescriptorBufferInfo bufferInfo {
        .buffer = mEnvMapViewsUBO.getBuffer(),
        .offset = 0,
        .range = VK_WHOLE_SIZE
    };

    VkDescriptorImageInfo imageInfo {
        .sampler = mEquirectangularTexture.vulkanSampler.sampler,
        .imageView = mEquirectangularTexture.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    std::array<VkWriteDescriptorSet, 2> dsWrites{};

    dsWrites.at(0).sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    dsWrites.at(0).dstSet = mCubemapConvertDs;
    dsWrites.at(0).dstBinding = 0;
    dsWrites.at(0).dstArrayElement = 0;
    dsWrites.at(0).descriptorCount = 1;
    dsWrites.at(0).descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsWrites.at(0).pBufferInfo = &bufferInfo;

    dsWrites.at(1).sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    dsWrites.at(1).dstSet = mCubemapConvertDs;
    dsWrites.at(1).dstBinding = 1;
    dsWrites.at(1).dstArrayElement = 0;
    dsWrites.at(1).descriptorCount = 1;
    dsWrites.at(1).descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    dsWrites.at(1).pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(mRenderDevice.device, dsWrites.size(), dsWrites.data(), 0, nullptr);

    setVulkanObjectDebugName(mRenderDevice,
                             VK_OBJECT_TYPE_DESCRIPTOR_SET,
                             "Renderer::mCubemapConvertDs",
                             mCubemapConvertDs);
}

void Renderer::createIrradianceConvolutionDs()
{
    vkFreeDescriptorSets(mRenderDevice.device, mRenderDevice.descriptorPool, 1, &mIrradianceConvolutionDs);

    VkDescriptorSetLayout dsLayout = mIrradianceConvolutionDsLayout;
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = mRenderDevice.descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &dsLayout
    };

    VkResult result = vkAllocateDescriptorSets(mRenderDevice.device, &descriptorSetAllocateInfo, &mIrradianceConvolutionDs);
    vulkanCheck(result, "Failed to allocate descriptor set.");

    setVulkanObjectDebugName(mRenderDevice,
                             VK_OBJECT_TYPE_DESCRIPTOR_SET,
                             "Renderer::mIrradianceConvolutionDs",
                             mIrradianceConvolutionDs);

    VkDescriptorBufferInfo bufferInfo {
        .buffer = mIrradianceViewsUBO.getBuffer(),
        .offset = 0,
        .range = VK_WHOLE_SIZE
    };

    VkDescriptorImageInfo imageInfo {
        .sampler = mEnvMap.vulkanSampler.sampler,
        .imageView = mEnvMap.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    std::array<VkWriteDescriptorSet, 2> dsWrites{};

    dsWrites.at(0).sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    dsWrites.at(0).dstSet = mIrradianceConvolutionDs;
    dsWrites.at(0).dstBinding = 0;
    dsWrites.at(0).dstArrayElement = 0;
    dsWrites.at(0).descriptorCount = 1;
    dsWrites.at(0).descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsWrites.at(0).pBufferInfo = &bufferInfo;

    dsWrites.at(1).sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    dsWrites.at(1).dstSet = mIrradianceConvolutionDs;
    dsWrites.at(1).dstBinding = 1;
    dsWrites.at(1).dstArrayElement = 0;
    dsWrites.at(1).descriptorCount = 1;
    dsWrites.at(1).descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    dsWrites.at(1).pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(mRenderDevice.device, dsWrites.size(), dsWrites.data(), 0, nullptr);

    setVulkanObjectDebugName(mRenderDevice,
                             VK_OBJECT_TYPE_DESCRIPTOR_SET,
                             "Renderer::mIrradianceConvolutionDs",
                             mIrradianceConvolutionDs);
}

void Renderer::executeCubemapConvertRenderpass()
{
    VkRenderPassBeginInfo renderPassBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mCubemapConvertRenderpass,
        .framebuffer = mCubemapConvertFramebuffer,
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = {
                .width = mEnvMapFaceSize,
                .height = mEnvMapFaceSize
            }
        },
        .clearValueCount = 0,
        .pClearValues = nullptr
    };

    VkViewport viewport {
        .x = 0.f,
        .y = static_cast<float>(mEnvMapFaceSize),
        .width = static_cast<float>(mEnvMapFaceSize),
        .height = -static_cast<float>(mEnvMapFaceSize),
        .minDepth = 0.f,
        .maxDepth = 1.f
    };

    VkRect2D scissor {
        .offset = {.x = 0, .y = 0},
        .extent = {
            .width = static_cast<uint32_t>(mEnvMapFaceSize),
            .height = static_cast<uint32_t>(mEnvMapFaceSize)
        }
    };

    VkCommandBuffer commandBuffer = beginSingleTimeCommands(mRenderDevice);

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mCubemapConvertPipeline);
    vkCmdBindDescriptorSets(commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            mCubemapConvertPipeline,
                            0, 1, &mCubemapConvertDs,
                            0, nullptr);
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);

    mEnvMap.transitionLayout(commandBuffer,
                             VK_IMAGE_LAYOUT_UNDEFINED,
                             VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0,
                             VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT);
    mEnvMap.generateMipMaps(commandBuffer);
    mEnvMap.transitionLayout(commandBuffer,
                             VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
                             VK_ACCESS_SHADER_READ_BIT);

    endSingleTimeCommands(mRenderDevice, commandBuffer);
}

void Renderer::executeIrradianceConvolutionRenderpass()
{
    VkRenderPassBeginInfo renderPassBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mIrradianceConvolutionRenderpass,
        .framebuffer = mIrradianceConvolutionFramebuffer,
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = {
                .width = static_cast<uint32_t>(mIrradianceMap.width),
                .height = static_cast<uint32_t>(mIrradianceMap.height)
            }
        },
        .clearValueCount = 0,
        .pClearValues = nullptr
    };

    VkCommandBuffer commandBuffer = beginSingleTimeCommands(mRenderDevice);

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mIrradianceConvolutionPipeline);
    vkCmdBindDescriptorSets(commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            mIrradianceConvolutionPipeline,
                            0, 1, &mIrradianceConvolutionDs,
                            0, nullptr);
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);

    endSingleTimeCommands(mRenderDevice, commandBuffer);
}

void Renderer::executePrefilterRenderpasses()
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(mRenderDevice);

    uint32_t width = mPrefilterMap.width;
    uint32_t height = mPrefilterMap.height;

    for (uint32_t i = 0; i < mPrefilterMap.mipLevels; ++i)
    {
        VkRenderPassBeginInfo renderPassBeginInfo {
            .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass = mPrefilterRenderpass,
            .framebuffer = mPrefilterFramebuffers.at(i),
            .renderArea = {
                .offset = {.x = 0, .y = 0},
                .extent = {
                    .width = width,
                    .height = height
                }
            },
            .clearValueCount = 0,
            .pClearValues = nullptr
        };

        VkViewport viewport {
            .x = 0.f,
            .y = static_cast<float>(height),
            .width = static_cast<float>(width),
            .height = -static_cast<float>(height),
            .minDepth = 0.f,
            .maxDepth = 1.f
        };

        VkRect2D scissor {
            .offset = {.x = 0, .y = 0},
            .extent = {
                .width = static_cast<uint32_t>(width),
                .height = static_cast<uint32_t>(height)
            }
        };

        float roughness = glm::clamp(static_cast<float>(i) / static_cast<float>(mMaxPrefilterMipLevels - 1), 0.f, 1.f);

        vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPrefilterPipeline);

        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                mPrefilterPipeline,
                                0, 1, &mIrradianceConvolutionDs,
                                0, nullptr);

        vkCmdPushConstants(commandBuffer,
                           mPrefilterPipeline,
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(float),
                           &roughness);

        vkCmdDraw(commandBuffer, 6, 1, 0, 0);
        vkCmdEndRenderPass(commandBuffer);

        width /= 2;
        height /= 2;
    }

    endSingleTimeCommands(mRenderDevice, commandBuffer);
}

void Renderer::executeBrdfLutRenderpass()
{
    VkRenderPassBeginInfo renderPassBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mBrdfLutRenderpass,
        .framebuffer = mBrdfLutFramebuffer,
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = {
                .width = static_cast<uint32_t>(mBrdfLut.width),
                .height = static_cast<uint32_t>(mBrdfLut.height)
            }
        },
        .clearValueCount = 0,
        .pClearValues = nullptr
    };

    VkCommandBuffer commandBuffer = beginSingleTimeCommands(mRenderDevice);

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mBrdfLutPipeline);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);

    endSingleTimeCommands(mRenderDevice, commandBuffer);
}

void Renderer::createBloomMipChain()
{
    TextureSpecification specification {
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .width = 0,
        .height = 0,
        .layerCount = 1,
        .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                      VK_IMAGE_USAGE_SAMPLED_BIT,
        .imageAspect = VK_IMAGE_ASPECT_COLOR_BIT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .magFilter = TextureMagFilter::Linear,
        .minFilter = TextureMinFilter::LinearMipmapNearest,
        .wrapS = TextureWrap::ClampToEdge,
        .wrapT = TextureWrap::ClampToEdge,
        .generateMipMaps = false
    };

    uint32_t width = mWidth;
    uint32_t height = mHeight;

    for (uint32_t i = 0; i < mBloomMipChain.size(); ++i)
    {
        specification.width = width;
        specification.height = height;

        mBloomMipChain.at(i) = {mRenderDevice, specification};
        mBloomMipChain.at(i).setDebugName(std::format("Renderer::mBloomMipChain.at({})", i));

        width = glm::max(1u, width / 2u);
        height = glm::max(1u, height / 2u);
    }
}

void Renderer::createCaptureBrightPixelsRenderpass()
{
    VkAttachmentDescription attachment {
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkAttachmentReference attachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentRef,
    };

    VkSubpassDependency dependency {
        .srcSubpass = 0,
        .dstSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
    };

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mCaptureBrightPixelsRenderpass);
    vulkanCheck(result, "Failed to create renderpass.");
    setRenderpassDebugName(mRenderDevice, mCaptureBrightPixelsRenderpass, "Renderer::mCaptureBrightPixelsRenderpass");
}

void Renderer::createCaptureBrightPixelsFramebuffer()
{
    vkDestroyFramebuffer(mRenderDevice.device, mCaptureBrightPixelsFramebuffer, nullptr);

    VkFramebufferCreateInfo framebufferCreateInfo {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = mCaptureBrightPixelsRenderpass,
        .attachmentCount = 1,
        .pAttachments = &mBloomMipChain.at(0).imageView,
        .width = mBloomMipChain.at(0).width,
        .height = mBloomMipChain.at(0).height,
        .layers = 1
    };

    VkResult result = vkCreateFramebuffer(mRenderDevice.device, &framebufferCreateInfo, nullptr, &mCaptureBrightPixelsFramebuffer);
    vulkanCheck(result, "Failed to create framebuffer.");
    setFramebufferDebugName(mRenderDevice, mCaptureBrightPixelsFramebuffer, "Renderer::mCaptureBrightPixelsFramebuffer");
}

void Renderer::createCaptureBrightPixelsPipeline()
{
    PipelineSpecification specification {
        .shaderStages = {
            .vertShaderPath = "shaders/fullscreen_render.vert.spv",
            .fragShaderPath = "shaders/bloom_prefilter.frag.spv"
        },
        .inputAssembly = {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        },
        .rasterization = {
            .rasterizerDiscardPrimitives = false,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .lineWidth = 1.f
        },
        .multisampling = {
            .samples = VK_SAMPLE_COUNT_1_BIT
        },
        .depthStencil = {
            .enableDepthTest = false,
            .enableDepthWrite = false
        },
        .blendStates = {
            {.enable = false}
        },
        .dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        },
        .pipelineLayout = {
            .dsLayouts = {mSingleImageDsLayout},
            .pushConstantRanges = {
                {
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .offset = 0,
                    .size = sizeof(float) * 3
                }
            }
        },
        .renderPass = mCaptureBrightPixelsRenderpass,
        .subpassIndex = 0,
        .debugName = "Renderer::mCaptureBrightPixelsPipeline"
    };

    mCaptureBrightPixelsPipeline = VulkanGraphicsPipeline(mRenderDevice, specification);
}

void Renderer::createBloomDownsampleRenderpass()
{
    VkAttachmentDescription attachment {
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkAttachmentReference attachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentRef,
    };

    VkSubpassDependency dependency {
        .srcSubpass = 0,
        .dstSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
    };

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mBloomDownsampleRenderpass);
    vulkanCheck(result, "Failed to create renderpass.");
    setRenderpassDebugName(mRenderDevice, mBloomDownsampleRenderpass, "Renderer::mBloomDownsampleRenderpass");
}

void Renderer::createBloomDownsampleFramebuffers()
{
    for (uint32_t i = 0; i < mBloomMipChain.size(); ++i)
    {
        vkDestroyFramebuffer(mRenderDevice.device, mBloomDownsampleFramebuffers.at(i), nullptr);

        VkFramebufferCreateInfo framebufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = mBloomDownsampleRenderpass,
            .attachmentCount = 1,
            .pAttachments = &mBloomMipChain.at(i).imageView,
            .width = mBloomMipChain.at(i).width,
            .height = mBloomMipChain.at(i).height,
            .layers = 1
        };

        VkResult result = vkCreateFramebuffer(mRenderDevice.device, &framebufferCreateInfo, nullptr, &mBloomDownsampleFramebuffers.at(i));
        vulkanCheck(result, "Failed to create framebuffer.");

        setFramebufferDebugName(mRenderDevice,
                                mBloomDownsampleFramebuffers.at(i),
                                std::format("Renderer::mBloomDownsampleFramebuffers.at({})", i));
    }
}

void Renderer::createBloomDownsamplePipeline()
{
    PipelineSpecification specification {
        .shaderStages = {
            .vertShaderPath = "shaders/fullscreen_render.vert.spv",
            .fragShaderPath = "shaders/bloom_downsample.frag.spv"
        },
        .inputAssembly = {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        },
        .rasterization = {
            .rasterizerDiscardPrimitives = false,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .lineWidth = 1.f
        },
        .multisampling = {
            .samples = VK_SAMPLE_COUNT_1_BIT
        },
        .depthStencil = {
            .enableDepthTest = false,
            .enableDepthWrite = false
        },
        .blendStates = {
            {.enable = false}
        },
        .dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        },
        .pipelineLayout = {
            .dsLayouts = {mSingleImageDsLayout},
            .pushConstantRanges = {
                {
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .offset = 0,
                    .size = sizeof(uint32_t) * 3
                }
            }
        },
        .renderPass = mBloomDownsampleRenderpass,
        .subpassIndex = 0,
        .debugName = "Renderer::mBloomDownsampleRenderpass"
    };

    mBloomDownsamplePipeline = VulkanGraphicsPipeline(mRenderDevice, specification);
}

void Renderer::createBloomUpsampleRenderpass()
{
    VkAttachmentDescription attachment {
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkAttachmentReference attachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentRef,
    };

    VkSubpassDependency dependency {
        .srcSubpass = 0,
        .dstSubpass = VK_SUBPASS_EXTERNAL,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
        .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
        .dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
    };

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mBloomUpsampleRenderpass);
    vulkanCheck(result, "Failed to create renderpass.");
    setRenderpassDebugName(mRenderDevice, mBloomUpsampleRenderpass, "Renderer::mBloomUpsampleRenderpass");
}

void Renderer::createBloomUpsampleFramebuffers()
{
    for (uint32_t i = 0; i < mBloomMipChain.size(); ++i)
    {
        vkDestroyFramebuffer(mRenderDevice.device, mBloomUpsampleFramebuffers.at(i), nullptr);

        VkFramebufferCreateInfo framebufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = mBloomUpsampleRenderpass,
            .attachmentCount = 1,
            .pAttachments = &mBloomMipChain.at(i).imageView,
            .width = mBloomMipChain.at(i).width,
            .height = mBloomMipChain.at(i).height,
            .layers = 1
        };

        VkResult result = vkCreateFramebuffer(mRenderDevice.device, &framebufferCreateInfo, nullptr, &mBloomUpsampleFramebuffers.at(i));
        vulkanCheck(result, "Failed to create framebuffer.");

        setFramebufferDebugName(mRenderDevice,
                                mBloomUpsampleFramebuffers.at(i),
                                std::format("Renderer::mBloomUpsampleFramebuffers.at({})", i));
    }
}

void Renderer::createBloomUpsamplePipeline()
{
    PipelineSpecification specification {
        .shaderStages = {
            .vertShaderPath = "shaders/fullscreen_render.vert.spv",
            .fragShaderPath = "shaders/bloom_upsample.frag.spv"
        },
        .inputAssembly = {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        },
        .rasterization = {
            .rasterizerDiscardPrimitives = false,
            .polygonMode = VK_POLYGON_MODE_FILL,
            .lineWidth = 1.f
        },
        .multisampling = {
            .samples = VK_SAMPLE_COUNT_1_BIT
        },
        .depthStencil = {
            .enableDepthTest = false,
            .enableDepthWrite = false
        },
        .blendStates = {
            {
                .enable = true,
                .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstColorBlendFactor = VK_BLEND_FACTOR_ONE,
                .colorBlendOp = VK_BLEND_OP_ADD,
                .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
                .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
                .alphaBlendOp = VK_BLEND_OP_ADD
            }
        },
        .dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        },
        .pipelineLayout = {
            .dsLayouts = {mSingleImageDsLayout},
            .pushConstantRanges = {
                {
                    .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                    .offset = 0,
                    .size = sizeof(float) * 3
                }
            }
        },
        .renderPass = mBloomUpsampleRenderpass,
        .subpassIndex = 0,
        .debugName = "Renderer::mBloomUpsamplePipeline"
    };

    mBloomUpsamplePipeline = VulkanGraphicsPipeline(mRenderDevice, specification);
}

void Renderer::createBloomMipChainDs()
{
    for (uint32_t i = 0; i < BloomMipChainSize; ++i)
    {
        vkFreeDescriptorSets(mRenderDevice.device,
                             mRenderDevice.descriptorPool,
                             1,
                             &mBloomMipChainDs.at(i));

        VkDescriptorSetLayout dsLayout = mSingleImageDsLayout;
        VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = mRenderDevice.descriptorPool,
            .descriptorSetCount = 1,
            .pSetLayouts = &dsLayout
        };

        VkResult result = vkAllocateDescriptorSets(mRenderDevice.device, &descriptorSetAllocateInfo, &mBloomMipChainDs.at(i));
        vulkanCheck(result, "Failed to allocate descriptor set.");

        setVulkanObjectDebugName(mRenderDevice,
                                 VK_OBJECT_TYPE_DESCRIPTOR_SET,
                                 std::format("Renderer::mBloomMipChainDs.at({})", i),
                                 mBloomMipChainDs.at(i));

        VkDescriptorImageInfo imageInfo {
            .sampler = mBloomMipChain.at(i).vulkanSampler.sampler,
            .imageView = mBloomMipChain.at(i).imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        VkWriteDescriptorSet writeDs {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = mBloomMipChainDs.at(i),
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &imageInfo,
        };

        vkUpdateDescriptorSets(mRenderDevice.device, 1, &writeDs, 0, nullptr);
    }
}

void Renderer::createSingleImageDs(VkDescriptorSet &ds, const VulkanTexture &texture, const char *name)
{
    VkDescriptorSetLayout dsLayout = mSingleImageDsLayout;

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = mRenderDevice.descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &dsLayout
    };

    VkResult result = vkAllocateDescriptorSets(mRenderDevice.device, &descriptorSetAllocateInfo, &ds);
    vulkanCheck(result, "Failed to allocate descriptor set.");
    setVulkanObjectDebugName(mRenderDevice, VK_OBJECT_TYPE_DESCRIPTOR_SET, name, ds);

    VkDescriptorImageInfo imageInfo {
        .sampler = texture.vulkanSampler.sampler,
        .imageView = texture.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkWriteDescriptorSet writeDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = ds,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &imageInfo
    };

    vkUpdateDescriptorSets(mRenderDevice.device, 1, &writeDescriptorSet, 0, nullptr);
}

void Renderer::createCameraDs()
{
    VkDescriptorSetLayout dsLayout = mCameraRenderDataDsLayout;

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = mRenderDevice.descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &dsLayout
    };

    VkResult result = vkAllocateDescriptorSets(mRenderDevice.device, &descriptorSetAllocateInfo, &mCameraDs);
    vulkanCheck(result, "Failed to allocate descriptor set.");

    VkDescriptorBufferInfo bufferInfo {
        .buffer = mCameraUBO.getBuffer(),
        .offset = 0,
        .range = sizeof(CameraRenderData)
    };

    VkWriteDescriptorSet writeDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = mCameraDs,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &bufferInfo
    };

    vkUpdateDescriptorSets(mRenderDevice.device, 1, &writeDescriptorSet, 0, nullptr);
}

void Renderer::createSingleImageDescriptorSets()
{
    createSingleImageDs(mSsaoTextureDs, mSsaoTexture, "Renderer::mSsaoTextureDs");
    createSingleImageDs(mSsaoBlurTexture1Ds, mSsaoBlurTexture1, "Renderer::mSsaoBlurTexture1Ds");
    createSingleImageDs(mSsaoBlurTexture2Ds, mSsaoBlurTexture2, "Renderer::mSsaoBlurTexture2Ds");
    createSingleImageDs(mDepthDs, mDepthTextureMS, "Renderer::mDepthDs");
    createSingleImageDs(mColorResolve32Ds, mColorResolve32, "Renderer::mColorResolve32");
    createSingleImageDs(mColor8UDs, mColorTexture8U, "Renderer::mColorTexture8U");
}

void Renderer::createSsaoDs()
{
    VkDescriptorSetLayout dsLayout = mSSAODsLayout;
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = mRenderDevice.descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &dsLayout
    };

    VkResult result = vkAllocateDescriptorSets(mRenderDevice.device, &descriptorSetAllocateInfo, &mSsaoDs);
    vulkanCheck(result, "Failed to allocate descriptor set.");
    setVulkanObjectDebugName(mRenderDevice, VK_OBJECT_TYPE_DESCRIPTOR_SET, "Renderer::mSsaoDs", mSsaoDs);

    VkDescriptorImageInfo posImageInfo {
        .sampler = mViewPosTexture.vulkanSampler.sampler,
        .imageView = mViewPosTexture.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkDescriptorImageInfo normalImageInfo {
        .sampler = mNormalTexture.vulkanSampler.sampler,
        .imageView = mNormalTexture.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkDescriptorImageInfo noiseImageInfo {
        .sampler = mSsaoNoiseTexture.vulkanSampler.sampler,
        .imageView = mSsaoNoiseTexture.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkDescriptorBufferInfo kernelBufferInfo {
        .buffer = mSsaoKernelSSBO.getBuffer(),
        .offset = 0,
        .range = VK_WHOLE_SIZE
    };

    VkWriteDescriptorSet writeDescriptorSetPrototype {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = mSsaoDs,
//        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
//        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
//        .pImageInfo = nullptr,
//        .pBufferInfo = nullptr
    };

    std::array<VkWriteDescriptorSet, 4> descriptorWrites {};
    descriptorWrites.fill(writeDescriptorSetPrototype);

    descriptorWrites.at(0).dstBinding = 0;
    descriptorWrites.at(0).descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites.at(0).pImageInfo = &posImageInfo;

    descriptorWrites.at(1).dstBinding = 1;
    descriptorWrites.at(1).descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites.at(1).pImageInfo = &normalImageInfo;

    descriptorWrites.at(2).dstBinding = 2;
    descriptorWrites.at(2).descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrites.at(2).pImageInfo = &noiseImageInfo;

    descriptorWrites.at(3).dstBinding = 3;
    descriptorWrites.at(3).descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites.at(3).pBufferInfo = &kernelBufferInfo;

    vkUpdateDescriptorSets(mRenderDevice.device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void Renderer::createLightsDs()
{
    VkDescriptorSetLayout dsLayout = mLightsDsLayout;
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = mRenderDevice.descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &dsLayout
    };

    VkResult result = vkAllocateDescriptorSets(mRenderDevice.device, &descriptorSetAllocateInfo, &mLightsDs);
    vulkanCheck(result, "Failed to allocate descriptor set.");

    setVulkanObjectDebugName(mRenderDevice,
                             VK_OBJECT_TYPE_DESCRIPTOR_SET,
                             "Renderer::mLightsDs",
                             mLightsDs);

    VkDescriptorBufferInfo dirLightBufferInfo {
        .buffer = mDirLightSSBO.getBuffer(),
        .offset = 0,
        .range = VK_WHOLE_SIZE
    };

    VkDescriptorBufferInfo pointLightBufferInfo {
        .buffer = mPointLightSSBO.getBuffer(),
        .offset = 0,
        .range = VK_WHOLE_SIZE
    };

    VkDescriptorBufferInfo spotLightBufferInfo {
        .buffer = mSpotLightSSBO.getBuffer(),
        .offset = 0,
        .range = VK_WHOLE_SIZE
    };

    VkDescriptorBufferInfo dirShadowOptionsBufferInfo {
        .buffer = mDirShadowDataSSBO.getBuffer(),
        .offset = 0,
        .range = VK_WHOLE_SIZE
    };

    VkDescriptorBufferInfo pointShadowOptionsBufferInfo {
        .buffer = mPointShadowDataSSBO.getBuffer(),
        .offset = 0,
        .range = VK_WHOLE_SIZE
    };

    VkDescriptorBufferInfo spotShadowOptionsBufferInfo {
        .buffer = mSpotShadowDataSSBO.getBuffer(),
        .offset = 0,
        .range = VK_WHOLE_SIZE
    };

    VkDescriptorImageInfo dirShadowSamplerImageInfo {
        .sampler = mDirShadowMapSampler.sampler,
        .imageView = VK_NULL_HANDLE,
    };

    VkDescriptorImageInfo pointShadowSamplerImageInfo {
        .sampler = mPointShadowMapSampler.sampler,
        .imageView = VK_NULL_HANDLE,
    };

    VkDescriptorImageInfo spotShadowSamplerImageInfo {
        .sampler = mSpotShadowMapSampler.sampler,
        .imageView = VK_NULL_HANDLE,
    };

    VkWriteDescriptorSet prototype {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = mLightsDs,
//        .dstBinding = x,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
//        .pBufferInfo = x
    };

    std::array<VkWriteDescriptorSet, 9> writeDs {};
    writeDs.fill(prototype);

    writeDs.at(0).dstBinding = 0;
    writeDs.at(0).pBufferInfo = &dirLightBufferInfo;

    writeDs.at(1).dstBinding = 1;
    writeDs.at(1).pBufferInfo = &pointLightBufferInfo;

    writeDs.at(2).dstBinding = 2;
    writeDs.at(2).pBufferInfo = &spotLightBufferInfo;

    writeDs.at(3).dstBinding = 3;
    writeDs.at(3).pBufferInfo = &dirShadowOptionsBufferInfo;

    writeDs.at(4).dstBinding = 4;
    writeDs.at(4).pBufferInfo = &pointShadowOptionsBufferInfo;

    writeDs.at(5).dstBinding = 5;
    writeDs.at(5).pBufferInfo = &spotShadowOptionsBufferInfo;

    writeDs.at(6).dstBinding = 6;
    writeDs.at(6).descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    writeDs.at(6).pImageInfo = &dirShadowSamplerImageInfo;

    writeDs.at(7).dstBinding = 7;
    writeDs.at(7).descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    writeDs.at(7).pImageInfo = &pointShadowSamplerImageInfo;

    writeDs.at(8).dstBinding = 8;
    writeDs.at(8).descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    writeDs.at(8).pImageInfo = &spotShadowSamplerImageInfo;

    vkUpdateDescriptorSets(mRenderDevice.device, writeDs.size(), writeDs.data(), 0, nullptr);
}

void Renderer::createFrustumClusterGenDs()
{
    VkDescriptorSetLayout dsLayout = mFrustumClusterGenDsLayout;

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = mRenderDevice.descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &dsLayout
    };

    VkResult result = vkAllocateDescriptorSets(mRenderDevice.device, &descriptorSetAllocateInfo, &mFrustumClusterGenDs);
    vulkanCheck(result, "Failed to allocate descriptor set.");

    setVulkanObjectDebugName(mRenderDevice,
                             VK_OBJECT_TYPE_DESCRIPTOR_SET,
                             "Renderer::mFrustumClusterGenDs",
                             mFrustumClusterGenDs);

    VkDescriptorBufferInfo bufferInfo {
        .buffer = mVolumeClustersSSBO.getBuffer(),
        .offset = 0,
        .range = VK_WHOLE_SIZE
    };

    VkWriteDescriptorSet writeDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = mFrustumClusterGenDs,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo = &bufferInfo
    };

    vkUpdateDescriptorSets(mRenderDevice.device, 1, &writeDescriptorSet, 0, nullptr);
}

void Renderer::createAssignLightsToClustersDs()
{
    VkDescriptorSetLayout dsLayout = mAssignLightsToClustersDsLayout;

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = mRenderDevice.descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &dsLayout
    };

    VkResult result = vkAllocateDescriptorSets(mRenderDevice.device, &descriptorSetAllocateInfo, &mAssignLightsToClustersDs);
    vulkanCheck(result, "Failed to allocate descriptor set.");

    setVulkanObjectDebugName(mRenderDevice,
                             VK_OBJECT_TYPE_DESCRIPTOR_SET,
                             "Renderer::mAssignLightsToClustersDs",
                             mAssignLightsToClustersDs);

    VkDescriptorBufferInfo clusterBufferInfo {
        .buffer = mVolumeClustersSSBO.getBuffer(),
        .offset = 0,
        .range = VK_WHOLE_SIZE
    };

    VkDescriptorBufferInfo pointLightBufferInfo {
        .buffer = mPointLightSSBO.getBuffer(),
        .offset = 0,
        .range = VK_WHOLE_SIZE
    };

    VkWriteDescriptorSet prototypeWriteDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = mAssignLightsToClustersDs,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo = nullptr
    };

    std::array<VkWriteDescriptorSet, 2> writeDs {};
    writeDs.fill(prototypeWriteDescriptorSet);

    writeDs.at(0).pBufferInfo = &clusterBufferInfo;
    writeDs.at(0).dstBinding = 0;

    writeDs.at(1).pBufferInfo = &pointLightBufferInfo;
    writeDs.at(1).dstBinding = 1;

    vkUpdateDescriptorSets(mRenderDevice.device, writeDs.size(), writeDs.data(), 0, nullptr);
}

void Renderer::createSkyboxDs()
{
    vkFreeDescriptorSets(mRenderDevice.device, mRenderDevice.descriptorPool, 1, &mSkyboxDs);

    VkDescriptorSetLayout dsLayout = mSingleImageDsLayout;
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = mRenderDevice.descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &dsLayout
    };

    VkResult result = vkAllocateDescriptorSets(mRenderDevice.device, &descriptorSetAllocateInfo, &mSkyboxDs);
    vulkanCheck(result, "Failed to allocate descriptor set.");

    setVulkanObjectDebugName(mRenderDevice,
                             VK_OBJECT_TYPE_DESCRIPTOR_SET,
                             "Renderer::mSkyboxDs",
                             mSkyboxDs);

    VkDescriptorImageInfo imageInfo {
        .sampler = mEnvMap.vulkanSampler.sampler,
        .imageView = mEnvMap.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkWriteDescriptorSet writeDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = mSkyboxDs,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &imageInfo
    };

    vkUpdateDescriptorSets(mRenderDevice.device, 1, &writeDescriptorSet, 0, nullptr);
}

void Renderer::createForwardShadingDs()
{
    VkDescriptorSetLayout dsLayout = mForwardShadingDsLayout;
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = mRenderDevice.descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &dsLayout
    };

    VkResult result = vkAllocateDescriptorSets(mRenderDevice.device, &descriptorSetAllocateInfo, &mForwardShadingDs);
    vulkanCheck(result, "Failed to allocate descriptor set.");

    setVulkanObjectDebugName(mRenderDevice,
                             VK_OBJECT_TYPE_DESCRIPTOR_SET,
                             "Renderer::mForwardShadingDs",
                             mForwardShadingDs);
}

void Renderer::updateForwardShadingDs()
{
    VkDescriptorImageInfo ssaoImageInfo {
        .sampler = mSsaoBlurTexture2.vulkanSampler.sampler,
        .imageView = mSsaoBlurTexture2.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkDescriptorBufferInfo clusterBufferInfo {
        .buffer = mVolumeClustersSSBO.getBuffer(),
        .offset = 0,
        .range = VK_WHOLE_SIZE
    };

    VkDescriptorImageInfo viewPosImageInfo {
        .sampler = mViewPosTexture.vulkanSampler.sampler,
        .imageView = mViewPosTexture.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkDescriptorImageInfo irradianceMapImageInfo {
        .sampler = mIrradianceMap.vulkanSampler.sampler,
        .imageView = mIrradianceMap.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkDescriptorImageInfo prefilterMapImageInfo {
        .sampler = mPrefilterMap.vulkanSampler.sampler,
        .imageView = mPrefilterMap.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkDescriptorImageInfo brdfLutImageInfo {
        .sampler = mBrdfLut.vulkanSampler.sampler,
        .imageView = mBrdfLut.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    std::array<VkWriteDescriptorSet, 6> dsWrites {};

    dsWrites.at(0).sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    dsWrites.at(0).dstSet = mForwardShadingDs;
    dsWrites.at(0).dstBinding = 0;
    dsWrites.at(0).dstArrayElement = 0;
    dsWrites.at(0).descriptorCount = 1;
    dsWrites.at(0).descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    dsWrites.at(0).pImageInfo = &ssaoImageInfo;

    dsWrites.at(1).sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    dsWrites.at(1).dstSet = mForwardShadingDs;
    dsWrites.at(1).dstBinding = 1;
    dsWrites.at(1).dstArrayElement = 0;
    dsWrites.at(1).descriptorCount = 1;
    dsWrites.at(1).descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    dsWrites.at(1).pBufferInfo = &clusterBufferInfo;

    dsWrites.at(2).sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    dsWrites.at(2).dstSet = mForwardShadingDs;
    dsWrites.at(2).dstBinding = 2;
    dsWrites.at(2).dstArrayElement = 0;
    dsWrites.at(2).descriptorCount = 1;
    dsWrites.at(2).descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    dsWrites.at(2).pImageInfo = &viewPosImageInfo;

    dsWrites.at(3).sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    dsWrites.at(3).dstSet = mForwardShadingDs;
    dsWrites.at(3).dstBinding = 3;
    dsWrites.at(3).dstArrayElement = 0;
    dsWrites.at(3).descriptorCount = 1;
    dsWrites.at(3).descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    dsWrites.at(3).pImageInfo = &irradianceMapImageInfo;

    dsWrites.at(4).sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    dsWrites.at(4).dstSet = mForwardShadingDs;
    dsWrites.at(4).dstBinding = 4;
    dsWrites.at(4).dstArrayElement = 0;
    dsWrites.at(4).descriptorCount = 1;
    dsWrites.at(4).descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    dsWrites.at(4).pImageInfo = &prefilterMapImageInfo;

    dsWrites.at(5).sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    dsWrites.at(5).dstSet = mForwardShadingDs;
    dsWrites.at(5).dstBinding = 5;
    dsWrites.at(5).dstArrayElement = 0;
    dsWrites.at(5).descriptorCount = 1;
    dsWrites.at(5).descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    dsWrites.at(5).pImageInfo = &brdfLutImageInfo;

    vkUpdateDescriptorSets(mRenderDevice.device, dsWrites.size(), dsWrites.data(), 0, nullptr);
}

void Renderer::createPostProcessingDs()
{
    vkFreeDescriptorSets(mRenderDevice.device, mRenderDevice.descriptorPool, 1, &mPostProcessingDs);

    VkDescriptorSetLayout dsLayout = mPostProcessingDsLayout;
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = mRenderDevice.descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &dsLayout
    };

    VkResult result = vkAllocateDescriptorSets(mRenderDevice.device, &descriptorSetAllocateInfo, &mPostProcessingDs);
    vulkanCheck(result, "Failed to allocate descriptor set.");

    setVulkanObjectDebugName(mRenderDevice,
                             VK_OBJECT_TYPE_DESCRIPTOR_SET,
                             "Renderer::mPostProcessingDs",
                             mPostProcessingDs);

    VkDescriptorImageInfo hdrImageInfo {
        .sampler = mColorResolve32.vulkanSampler.sampler,
        .imageView = mColorResolve32.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    VkDescriptorImageInfo bloomImageInfo {
        .sampler = mBloomMipChain.front().vulkanSampler.sampler,
        .imageView = mBloomMipChain.front().imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
    };

    std::array<VkWriteDescriptorSet, 2> dsWrites {};

    dsWrites.at(0).sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    dsWrites.at(0).dstSet = mPostProcessingDs,
    dsWrites.at(0).dstBinding = 0,
    dsWrites.at(0).dstArrayElement = 0,
    dsWrites.at(0).descriptorCount = 1,
    dsWrites.at(0).descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    dsWrites.at(0).pImageInfo = &hdrImageInfo;

    dsWrites.at(1).sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
    dsWrites.at(1).dstSet = mPostProcessingDs,
    dsWrites.at(1).dstBinding = 1,
    dsWrites.at(1).dstArrayElement = 0,
    dsWrites.at(1).descriptorCount = 1,
    dsWrites.at(1).descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    dsWrites.at(1).pImageInfo = &bloomImageInfo;

    vkUpdateDescriptorSets(mRenderDevice.device, dsWrites.size(), dsWrites.data(), 0, nullptr);
}

void Renderer::importModels()
{
    {
        ModelImportData importData {
            .path = "assets/models/damaged_helmet/DamagedHelmet.gltf",
            .normalize = false,
            .flipUVs = true
        };

        importModel(importData);
    }

    {
        ModelImportData importData {
            .path = "assets/models/egyptian_cat_statue/egyptian_cat.gltf",
            .normalize = true,
            .flipUVs = true
        };

        importModel(importData);
    }

    {
        ModelImportData importData {
            .path = "assets/models/skull/skull.gltf",
            .normalize = true,
            .flipUVs = true
        };

        importModel(importData);
    }
}
