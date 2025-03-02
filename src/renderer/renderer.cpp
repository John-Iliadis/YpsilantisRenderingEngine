//
// Created by Gianni on 4/01/2025.
//

#include "renderer.hpp"

Renderer::Renderer(const VulkanRenderDevice& renderDevice, SaveData& saveData)
    : SubscriberSNS({Topic::Type::Resource})
    , mRenderDevice(renderDevice)
    , mSaveData(saveData)
    , mWidth(InitialViewportWidth)
    , mHeight(InitialViewportHeight)
    , mCameraUBO(renderDevice, sizeof(CameraRenderData), BufferType::Uniform, MemoryType::HostCoherent)
{
    if (saveData.contains("viewport"))
    {
        mWidth = saveData["viewport"]["width"];
        mHeight = saveData["viewport"]["height"];
    }

    mCamera = Camera(glm::vec3(0.f, 1.f, 0.f), 30.f, mWidth, mHeight, 0.01f);

    createDefaultMaterialTextures(mRenderDevice);

    createColorTexture32F();
    createDepthTexture();
    createNormalTexture();
    createSsaoTexture();
    createColorTexture8U();
    createIconDepthTexture();

    createSingleImageDsLayout();
    createCameraRenderDataDsLayout();
    createMaterialsDsLayout();
    createDepthNormalInputDsLayout();

    createCameraDs();
    createSingleImageDescriptorSets();
    createDepthNormalDs();

    createPrepassRenderpass();
    createPrepassFramebuffer();
    createPrepassPipeline();

    createSkyboxRenderpass();
    createSkyboxFramebuffer();
    createSkyboxPipeline();

    createSsaoRenderpass();
    createSsaoFramebuffer();
    createSsaoPipeline();

    createOitTextures();
    createOitBuffers();
    createOitResourcesDs();
    updateOitResourcesDs();
    createForwardRenderpass();
    createForwardFramebuffer();
    createOitTransparentCollectionPipeline();
    createOitTransparencyResolutionPipeline();
    createOpaqueForwardPassPipeline();
    createForwardPassBlendPipeline();

    createGridRenderpass();
    createGridFramebuffer();
    createGridPipeline();

    createPostProcessingRenderpass();
    createPostProcessingFramebuffer();
    createPostProcessingPipeline();

    createLightBuffers();
    createLightIconTextures();
    createLightIconTextureDsLayout();
    createLightIconRenderpass();
    createLightIconFramebuffer();
    createLightIconPipeline();

    loadSkybox();
}

Renderer::~Renderer()
{
    destroyDefaultMaterialTextures();

    vkDestroyFramebuffer(mRenderDevice.device, mPrepassFramebuffer, nullptr);
    vkDestroyFramebuffer(mRenderDevice.device, mSkyboxFramebuffer, nullptr);
    vkDestroyFramebuffer(mRenderDevice.device, mForwardPassFramebuffer, nullptr);
    vkDestroyFramebuffer(mRenderDevice.device, mSsaoFramebuffer, nullptr);
    vkDestroyFramebuffer(mRenderDevice.device, mGridFramebuffer, nullptr);
    vkDestroyFramebuffer(mRenderDevice.device, mPostProcessingFramebuffer, nullptr);
    vkDestroyFramebuffer(mRenderDevice.device, mLightIconFramebuffer, nullptr);

    vkDestroyRenderPass(mRenderDevice.device, mPrepassRenderpass, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mSkyboxRenderpass, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mSsaoRenderpass, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mForwardRenderpass, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mGridRenderpass, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mPostProcessingRenderpass, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mLightIconRenderpass, nullptr);
}

void Renderer::update()
{
    updateCameraUBO();
}

void Renderer::render(VkCommandBuffer commandBuffer)
{
    setViewport(commandBuffer);

    executePrepass(commandBuffer);
    executeSkyboxRenderpass(commandBuffer);
    executeSsaoRenderpass(commandBuffer);
    executeForwardRenderpass(commandBuffer);
    executeGridRenderpass(commandBuffer);
    executePostProcessingRenderpass(commandBuffer);
    executeLightIconRenderpass(commandBuffer);
}

void Renderer::importModel(const ModelImportData& importData)
{
    bool loaded = std::find_if(mModels.begin(), mModels.end(), [&importData] (const auto& pair) {
        return importData.path == pair.second->path;
    }) != mModels.end();

    if (loaded)
    {
        debugLog("Model is already loaded.");
        return;
    }

    SNS::publishMessage(Topic::Type::Renderer,
                        Message::loadModel(importData.path,
                                           importData.normalize,
                                           importData.flipUVs));
}

void Renderer::importSkybox(const std::array<std::string, 6> &paths)
{
    CubemapLoader loader(mRenderDevice, paths);

    if (!loader.success()) return;

    loader.get(mSkyboxTexture);

    VkDescriptorImageInfo imageInfo {
        .sampler = mSkyboxTexture.vulkanSampler.sampler,
        .imageView = mSkyboxTexture.imageView,
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

void Renderer::deleteModel(uuid32_t id)
{
    SNS::publishMessage(Topic::Type::Renderer, Message::modelDeleted(id));
    mModels.erase(id);
}

void Renderer::resize(uint32_t width, uint32_t height)
{
    mWidth = width;
    mHeight = height;

    mCamera.resize(width, height);

    std::vector<VkDescriptorSet> freeDs {
        mDepthNormalDs,
        mSsaoDs,
        mDepthDs,
        mColor32FDs,
        mColor8UDs,
    };

    vkFreeDescriptorSets(mRenderDevice.device, mRenderDevice.descriptorPool, freeDs.size(), freeDs.data());

    createOitTextures();
    createColorTexture32F();
    createDepthTexture();
    createNormalTexture();
    createSsaoTexture();
    createColorTexture8U();
    createIconDepthTexture();

    createPrepassFramebuffer();
    createSkyboxFramebuffer();
    createSsaoFramebuffer();
    createForwardFramebuffer();
    createGridFramebuffer();
    createPostProcessingFramebuffer();
    createLightIconFramebuffer();

    createSingleImageDescriptorSets();
    createDepthNormalDs();

    createOitBuffers();
    updateOitResourcesDs();
}

void Renderer::notify(const Message &message)
{
    if (const auto m = message.getIf<Message::ModelLoaded>())
    {
        auto model = m->model;

        mModels.emplace(model->id, model);

        model->createMaterialsUBO();
        model->createTextureDescriptorSets(mSingleImageDsLayout);
    }
}

void Renderer::addDirLight(uuid32_t id, const DirectionalLight &light)
{
    addLight(mUuidToDirLightIndex, mDirectionalLights, mDirLightSSBO, id, light);
}

void Renderer::addPointLight(uuid32_t id, const PointLight &light)
{
    addLight(mUuidToPointLightIndex, mPointLights, mPointLightSSBO, id, light);
}

void Renderer::addSpotLight(uuid32_t id, const SpotLight &light)
{
    addLight(mUuidToSpotLightIndex, mSpotLights, mSpotLightSSBO, id, light);
}

DirectionalLight &Renderer::getDirLight(uuid32_t id)
{
    return getLight(mUuidToDirLightIndex, mDirectionalLights, id);
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
    updateLight(mUuidToDirLightIndex, mDirectionalLights, mDirLightSSBO, id);
}

void Renderer::updateSpotLight(uuid32_t id)
{
    updateLight(mUuidToSpotLightIndex, mSpotLights, mSpotLightSSBO, id);
}

void Renderer::updatePointLight(uuid32_t id)
{
    updateLight(mUuidToPointLightIndex, mPointLights, mPointLightSSBO, id);
}

void Renderer::deleteDirLight(uuid32_t id)
{
    deleteLight(mUuidToDirLightIndex, mDirectionalLights, mDirLightSSBO, id);
}

void Renderer::deletePointLight(uuid32_t id)
{
    deleteLight(mUuidToPointLightIndex, mPointLights, mPointLightSSBO, id);
}

void Renderer::deleteSpotLight(uuid32_t id)
{
    deleteLight(mUuidToSpotLightIndex, mSpotLights, mSpotLightSSBO, id);
}

void Renderer::executePrepass(VkCommandBuffer commandBuffer)
{
    static constexpr VkClearValue normalClear {.color = {0.f, 0.f, 0.f, 0.f}};
    static constexpr VkClearValue depthClear {.depthStencil = {.depth = 1.f, .stencil = 0}};
    static constexpr std::array<VkClearValue, 2> clears {normalClear, depthClear};

    VkRenderPassBeginInfo renderPassBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mPrepassRenderpass,
        .framebuffer = mPrepassFramebuffer,
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = {.width = mWidth, .height = mHeight}
        },
        .clearValueCount = static_cast<uint32_t>(clears.size()),
        .pClearValues = clears.data()
    };

    beginDebugLabel(commandBuffer, "Prepass");
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPrepassPipeline);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            mPrepassPipeline,
                            0, 1, &mCameraDs,
                            0, nullptr);

    renderModels(commandBuffer, mPrepassPipeline, 1, true);

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
            .extent = {.width = mWidth, .height = mHeight}
        },
        .clearValueCount = 1,
        .pClearValues = &colorClear
    };

    beginDebugLabel(commandBuffer, "Skybox Renderpass");
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    if (mRenderSkybox)
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mSkyboxPipeline);

        std::array<VkDescriptorSet, 2> descriptorSets {mCameraDs, mSkyboxDs};
        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                mSkyboxPipeline,
                                0, descriptorSets.size(), descriptorSets.data(),
                                0, nullptr);

        vkCmdPushConstants(commandBuffer,
                           mSkyboxPipeline,
                           VK_SHADER_STAGE_FRAGMENT_BIT,
                           0, sizeof(int32_t) * 3,
                           &mSkyboxData);

        vkCmdDraw(commandBuffer, 36, 1, 0, 0);
    }

    vkCmdEndRenderPass(commandBuffer);
    endDebugLabel(commandBuffer);
}

void Renderer::executeSsaoRenderpass(VkCommandBuffer commandBuffer)
{
    VkClearValue ssaoClear {.color = {0.f}};

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
        mDepthNormalDs
    };

    beginDebugLabel(commandBuffer, "SSAO Renderpass");
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mSsaoPipeline);

    vkCmdBindDescriptorSets(commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            mSsaoPipeline,
                            0, descriptorSets.size(), descriptorSets.data(),
                            0, nullptr);

    if (mSsaoOn) vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
    endDebugLabel(commandBuffer);
}

void Renderer::executeForwardRenderpass(VkCommandBuffer commandBuffer)
{
    static constexpr VkClearValue transparencyClear {.color = {0.f, 0.f, 0.f, 0.f}};

    VkRenderPassBeginInfo renderPassBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mForwardRenderpass,
        .framebuffer = mForwardPassFramebuffer,
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = {.width = mWidth, .height = mHeight}
        },
        .clearValueCount = 1,
        .pClearValues = &transparencyClear
    };

    beginDebugLabel(commandBuffer, "Forward Pass");

    { // clear oit resources
        static constexpr VkClearColorValue clearColor {.uint32 = {0xffffffff}};
        static constexpr VkImageSubresourceRange imageSubresourceRange {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .levelCount = 1,
            .layerCount = 1,
        };

        vkCmdClearColorImage(commandBuffer,
                             mTransparentNodeIndexStorageImage.image,
                             VK_IMAGE_LAYOUT_GENERAL,
                             &clearColor,
                             1, &imageSubresourceRange);

        vkCmdFillBuffer(commandBuffer, mOitLinkedListInfoSSBO.getBuffer(), 0, sizeof(uint32_t), 0);

        VkMemoryBarrier memoryBarrier {
            .sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT,
            .dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT
        };

        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 1, &memoryBarrier,
                             0, nullptr, 0, nullptr);
    }

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    { // subpass 0: opaque pass
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mOpaqueForwardPassPipeline);

        std::array<VkDescriptorSet, 2> ds {mCameraDs, mSsaoDs};
        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                mOpaqueForwardPassPipeline,
                                0, ds.size(), ds.data(),
                                0, nullptr);

        renderModels(commandBuffer, mOpaqueForwardPassPipeline, 2, true);
    }

    { // subpass 1: transparent fragment collection
        vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mTransparentCollectionPipeline);

        std::array<VkDescriptorSet, 2> ds {mCameraDs, mOitResourcesDs};
        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                mTransparentCollectionPipeline,
                                0, ds.size(), ds.data(),
                                0, nullptr);

        if (mOitOn) renderModels(commandBuffer, mTransparentCollectionPipeline, 2, false);
    }

    { // subpass 2: transparency resolution
        vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mTransparencyResolutionPipeline);
        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                mTransparencyResolutionPipeline,
                                0, 1, &mOitResourcesDs,
                                0, nullptr);
        if (mOitOn) vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    }

    { // subpass 3: blend pass
        vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mForwardPassBlendPipeline);
        vkCmdBindDescriptorSets(commandBuffer,
                                VK_PIPELINE_BIND_POINT_GRAPHICS,
                                mForwardPassBlendPipeline,
                                0, 1, &mTransparentTexInputAttachmentDs,
                                0, nullptr);
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
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
            .extent = {.width = mWidth, .height = mHeight}
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

void Renderer::executePostProcessingRenderpass(VkCommandBuffer commandBuffer)
{
    VkClearValue clearValue {.color = {1.f, 1.f, 1.f, 1.f}};

    VkRenderPassBeginInfo renderPassBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mPostProcessingRenderpass,
        .framebuffer = mPostProcessingFramebuffer,
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = {.width = mWidth, .height = mHeight}
        },
        .clearValueCount = 1,
        .pClearValues = &clearValue
    };

    beginDebugLabel(commandBuffer, "Post Processing Renderpass");
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPostProcessingPipeline);

    vkCmdBindDescriptorSets(commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            mPostProcessingPipeline,
                            0, 1, &mColor32FDs,
                            0, nullptr);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
    endDebugLabel(commandBuffer);
}

void Renderer::executeLightIconRenderpass(VkCommandBuffer commandBuffer)
{
    std::array<VkClearValue, 2> clearValues;
    clearValues.at(1) = {.depthStencil = {.depth = 1.f}};

    VkRenderPassBeginInfo renderPassBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mLightIconRenderpass,
        .framebuffer = mLightIconFramebuffer,
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = {.width = mWidth, .height = mHeight}
        },
        .clearValueCount = static_cast<uint32_t>(clearValues.size()),
        .pClearValues = clearValues.data()
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

    bindTexture(commandBuffer, mLightIconPipeline, mDepthTexture, 1, 1);
    for (const auto& renderData : mLightIconRenderData)
    {
        bindTexture(commandBuffer, mLightIconPipeline, *renderData.icon, 0, 1);
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

void Renderer::getLightIconRenderData()
{
    mLightIconRenderData.clear();
    for (const auto& [id, index] : mUuidToDirLightIndex)
        mLightIconRenderData.emplace_back(mSceneGraph.searchNode(id)->globalT,
                                          &mDirLightIcon);
    for (const auto& [id, index] : mUuidToPointLightIndex)
        mLightIconRenderData.emplace_back(mSceneGraph.searchNode(id)->globalT,
                                          &mPointLightIcon);
    for (const auto& [id, index] : mUuidToSpotLightIndex)
        mLightIconRenderData.emplace_back(mSceneGraph.searchNode(id)->globalT,
                                          &mSpotLightIcon);

    std::sort(mLightIconRenderData.begin(), mLightIconRenderData.end(),
              [this] (const auto& a, const auto& b) {
        return glm::length(a.pos - mCamera.position()) > glm::length(b.pos - mCamera.position());
    });
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
        .extent = {.width = mWidth, .height = mHeight}
    };

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void Renderer::renderModels(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t matDsIndex, bool opaque)
{
    for (const auto& [id, model] : mModels)
    {
        if (opaque)
        {
            pfnCmdSetCullModeEXT(commandBuffer, model->cullMode);
            pfnCmdSetFrontFaceEXT(commandBuffer, model->frontFace);
        }

        for (const auto& mesh : model->meshes)
        {
            if (model->drawOpaque(mesh) == opaque)
            {
                uint32_t materialIndex = mesh.materialIndex;

                model->bindMaterialUBO(commandBuffer, pipelineLayout, materialIndex, matDsIndex);
                model->bindTextures(commandBuffer, pipelineLayout, materialIndex, matDsIndex);

                mesh.mesh.render(commandBuffer);
            }
        }
    }
}

void Renderer::updateCameraUBO()
{
    CameraRenderData renderData(mCamera.renderData());
    mCameraUBO.mapBufferMemory(0, sizeof(CameraRenderData), &renderData);
}

void Renderer::bindTexture(VkCommandBuffer commandBuffer,
                           VkPipelineLayout pipelineLayout,
                           const VulkanTexture &texture,
                           uint32_t binding,
                           uint32_t set)
{
    VkDescriptorImageInfo imageInfo(texture.vulkanSampler.sampler, texture.imageView, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

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

void Renderer::createColorTexture32F()
{
    TextureSpecification specification {
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
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

    mColorTexture32F = VulkanTexture(mRenderDevice, specification);
    mColorTexture32F.setDebugName("Renderer::mColorTexture32F");
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

void Renderer::createDepthTexture()
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
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .magFilter = TextureMagFilter::Nearest,
        .minFilter = TextureMinFilter::Nearest,
        .wrapS = TextureWrap::ClampToEdge,
        .wrapT = TextureWrap::ClampToEdge,
        .generateMipMaps = false
    };

    mDepthTexture = VulkanTexture(mRenderDevice, specification);
    mDepthTexture.setDebugName("Renderer::mDepthTexture");
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

void Renderer::createSsaoTexture()
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
}

void Renderer::createIconDepthTexture()
{
    TextureSpecification specification {
        .format = VK_FORMAT_D32_SFLOAT,
        .width = mWidth,
        .height = mHeight,
        .layerCount = 1,
        .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
        .imageUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .imageAspect = VK_IMAGE_ASPECT_DEPTH_BIT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .magFilter = TextureMagFilter::Nearest,
        .minFilter = TextureMinFilter::Nearest,
        .wrapS = TextureWrap::ClampToEdge,
        .wrapT = TextureWrap::ClampToEdge,
        .generateMipMaps = false
    };

    mIconDepthTexture = VulkanTexture(mRenderDevice, specification);
    mIconDepthTexture.setDebugName("Renderer::mIconDepthTexture");
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
            binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT)
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

void Renderer::createDepthNormalInputDsLayout()
{
    DsLayoutSpecification specification {
        .bindings = {
            binding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
        },
        .debugName = "Renderer::mDepthNormalDsLayout"
    };

    mDepthNormalDsLayout = VulkanDsLayout(mRenderDevice, specification);
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
    createSingleImageDs(mSsaoDs, mSsaoTexture, "Renderer::mSsaoDs");
    createSingleImageDs(mDepthDs, mDepthTexture, "Renderer::mDepthDs");
    createSingleImageDs(mColor32FDs, mColorTexture32F, "Renderer::mColor32FDs");
    createSingleImageDs(mColor8UDs, mColorTexture8U, "Renderer::mColorTexture8U");
}

void Renderer::createDepthNormalDs()
{
    VkDescriptorSetLayout dsLayout = mDepthNormalDsLayout;

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = mRenderDevice.descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &dsLayout
    };

    VkResult result = vkAllocateDescriptorSets(mRenderDevice.device, &descriptorSetAllocateInfo, &mDepthNormalDs);
    vulkanCheck(result, "Failed to allocate descriptor set.");

    VkDescriptorImageInfo depthImageInfo {
        .sampler = mDepthTexture.vulkanSampler.sampler,
        .imageView = mDepthTexture.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkDescriptorImageInfo normalImageInfo {
        .sampler = mNormalTexture.vulkanSampler.sampler,
        .imageView = mNormalTexture.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkWriteDescriptorSet writeDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = mDepthNormalDs,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &depthImageInfo
    };

    vkUpdateDescriptorSets(mRenderDevice.device, 1, &writeDescriptorSet, 0, nullptr);

    writeDescriptorSet.dstBinding = 1;
    writeDescriptorSet.pImageInfo = &normalImageInfo;
    vkUpdateDescriptorSets(mRenderDevice.device, 1, &writeDescriptorSet, 0, nullptr);
}

void Renderer::createPrepassRenderpass()
{
    VkAttachmentDescription normalAttachment {
        .format = mNormalTexture.format,
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
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    std::array<VkAttachmentDescription, 2> attachments {{
        normalAttachment,
        depthAttachment,
    }};

    VkAttachmentReference normalAttachmentRef {
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
        .pColorAttachments = &normalAttachmentRef,
        .pDepthStencilAttachment = &depthAttachmentRef
    };

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mPrepassRenderpass);
    vulkanCheck(result, "Failed to create renderpass.");
    setRenderpassDebugName(mRenderDevice, mPrepassRenderpass, "Renderer::mPrepassRenderpass");
}

void Renderer::createPrepassFramebuffer()
{
    vkDestroyFramebuffer(mRenderDevice.device, mPrepassFramebuffer, nullptr);

    std::array<VkImageView, 2> imageViews {
        mNormalTexture.imageView,
        mDepthTexture.imageView,
    };

    VkFramebufferCreateInfo framebufferCreateInfo {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = mPrepassRenderpass,
        .attachmentCount = static_cast<uint32_t>(imageViews.size()),
        .pAttachments = imageViews.data(),
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
            .samples = VK_SAMPLE_COUNT_1_BIT
        },
        .depthStencil = {
            .enableDepthTest = VK_TRUE,
            .enableDepthWrite = VK_TRUE,
            .depthCompareOp = VK_COMPARE_OP_LESS
        },
        .blending = {
            .enable = false
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
                mMaterialsDsLayout
            }
        },
        .renderPass = mPrepassRenderpass,
        .subpassIndex = 0,
        .debugName = "Renderer::mPrepassPipeline"
    };

    mPrepassPipeline = VulkanGraphicsPipeline(mRenderDevice, specification);
}

void Renderer::createSkyboxRenderpass()
{
    VkAttachmentDescription colorAttachment {
        .format = mColorTexture32F.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentDescription depthAttachment {
        .format = mDepthTexture.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
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

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mSkyboxRenderpass);
    vulkanCheck(result, "Failed to create renderpass.");
    setRenderpassDebugName(mRenderDevice, mSkyboxRenderpass, "Renderer::mSkyboxRenderpass");
}

void Renderer::createSkyboxFramebuffer()
{
    vkDestroyFramebuffer(mRenderDevice.device, mSkyboxFramebuffer, nullptr);

    std::array<VkImageView, 2> imageViews {
        mColorTexture32F.imageView,
        mDepthTexture.imageView,
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
            .samples = VK_SAMPLE_COUNT_1_BIT
        },
        .depthStencil = {
            .enableDepthTest = VK_TRUE,
            .enableDepthWrite = VK_FALSE,
            .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL
        },
        .blending = {
            .enable = false
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
            .pushConstantRange = VkPushConstantRange {
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = 0,
                .size = sizeof(int32_t) * 3
            }
        },
        .renderPass = mSkyboxRenderpass,
        .subpassIndex = 0,
        .debugName = "Renderer::mSkyboxPipeline"
    };

    mSkyboxPipeline = VulkanGraphicsPipeline(mRenderDevice, specification);
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

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &ssaoAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
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
        .blending = {
            .enable = false
        },
        .dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        },
        .pipelineLayout = {
            .dsLayouts = {
                mCameraRenderDataDsLayout,
                mDepthNormalDsLayout
            }
        },
        .renderPass = mSsaoRenderpass,
        .subpassIndex = 0,
        .debugName = "Renderer::mSsaoPipeline"
    };

    mSsaoPipeline = VulkanGraphicsPipeline(mRenderDevice, specification);
}

void Renderer::createOitTextures()
{
    mTransparentNodeIndexStorageImage = {
        mRenderDevice,
        VK_IMAGE_VIEW_TYPE_2D,
        VK_FORMAT_R32_UINT,
        mWidth, mHeight,
        VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        VK_IMAGE_ASPECT_COLOR_BIT
    };

    mTransparentNodeIndexStorageImage.transitionLayout(
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_GENERAL,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);
    mTransparentNodeIndexStorageImage.setDebugName("Renderer::mTransparentNodeIndexStorageImage");

    TextureSpecification transparencyTexSpecification {
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .width = mWidth,
        .height = mHeight,
        .layerCount = 1,
        .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
            VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT,
        .imageAspect = VK_IMAGE_ASPECT_COLOR_BIT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .magFilter = TextureMagFilter::Nearest,
        .minFilter = TextureMinFilter::Nearest,
        .wrapS = TextureWrap::ClampToEdge,
        .wrapT = TextureWrap::ClampToEdge,
        .generateMipMaps = false
    };

    mTransparencyTexture = {mRenderDevice, transparencyTexSpecification};
    mTransparencyTexture.setDebugName("Renderer::mTransparencyTexture");
}

void Renderer::createOitBuffers()
{
    uint32_t ssboMemSize = mWidth * mHeight * sizeof(TransparentNode) * mOitLinkedListLength;
    uint32_t data[2] = {0, mWidth * mHeight * mOitLinkedListLength};

    mOitLinkedListInfoSSBO = {mRenderDevice, 2 * sizeof(uint32_t), BufferType::Storage, MemoryType::Device};
    mOitLinkedListInfoSSBO.update(0, sizeof(data), data);

    mOitLinkedListSSBO = {mRenderDevice, ssboMemSize, BufferType::Storage, MemoryType::Device};
}

void Renderer::createOitResourcesDs()
{
    // mOitResourcesDsLayout
    DsLayoutSpecification mOitResourcesDsLayoutSpecification {
        .bindings = {
            binding(0, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
            binding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT),
        },
        .debugName = "Renderer::mOitResourcesDs"
    };

    mOitResourcesDsLayout = {mRenderDevice, mOitResourcesDsLayoutSpecification};

    // mSingleInputAttachmentDsLayout
    DsLayoutSpecification mSingleInputAttachmentDsLayoutSpecification {
        .bindings = {
            binding(0, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
        },
        .debugName = "Renderer::mSingleInputAttachmentDsLayout"
    };

    mSingleInputAttachmentDsLayout = {mRenderDevice, mSingleInputAttachmentDsLayoutSpecification};

    // mOitResourcesDs + mTransparentTexInputAttachmentDs allocation
    std::array<VkDescriptorSetLayout, 2> dsLayouts {
        mOitResourcesDsLayout,
        mSingleInputAttachmentDsLayout
    };

    VkDescriptorSetAllocateInfo dsAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = mRenderDevice.descriptorPool,
        .descriptorSetCount = static_cast<uint32_t>(dsLayouts.size()),
        .pSetLayouts = dsLayouts.data()
    };

    std::array<VkDescriptorSet, 2> descriptorSets;
    VkResult result = vkAllocateDescriptorSets(mRenderDevice.device, &dsAllocateInfo, descriptorSets.data());
    vulkanCheck(result, "Failed to allocate descriptor sets");

    mOitResourcesDs = descriptorSets.at(0);
    mTransparentTexInputAttachmentDs = descriptorSets.at(1);

    setVulkanObjectDebugName(mRenderDevice,
                             VK_OBJECT_TYPE_DESCRIPTOR_SET,
                             "Renderer::mOitResourcesDs",
                             mOitResourcesDs);

    setVulkanObjectDebugName(mRenderDevice,
                             VK_OBJECT_TYPE_DESCRIPTOR_SET,
                             "Renderer::mTransparentTexInputAttachmentDs",
                             mTransparentTexInputAttachmentDs);
}

void Renderer::updateOitResourcesDs()
{
    // mOitResourcesDs + mTransparentTexInputAttachmentDs
    VkDescriptorImageInfo imageInfo {
        .imageView = mTransparentNodeIndexStorageImage.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_GENERAL
    };

    VkDescriptorBufferInfo oitLinkedListSSBODescriptorInfo {
        .buffer = mOitLinkedListSSBO.getBuffer(),
        .offset = 0,
        .range = VK_WHOLE_SIZE
    };

    VkDescriptorBufferInfo oitLinkedListInfoSSBODescriptorInfo {
        .buffer = mOitLinkedListInfoSSBO.getBuffer(),
        .offset = 0,
        .range = VK_WHOLE_SIZE
    };

    VkDescriptorImageInfo transparentTexImageInfo {
        .sampler = mTransparencyTexture.vulkanSampler.sampler,
        .imageView = mTransparencyTexture.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    std::array<VkWriteDescriptorSet, 4> descriptorWrites {{
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = mOitResourcesDs,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
            .pImageInfo = &imageInfo
        },
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = mOitResourcesDs,
            .dstBinding = 1,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &oitLinkedListSSBODescriptorInfo
        },
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = mOitResourcesDs,
            .dstBinding = 2,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .pBufferInfo = &oitLinkedListInfoSSBODescriptorInfo
        },
        {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = mTransparentTexInputAttachmentDs,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT,
            .pImageInfo = &transparentTexImageInfo
        },
    }};

    vkUpdateDescriptorSets(mRenderDevice.device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void Renderer::createForwardRenderpass()
{
    VkAttachmentDescription transparentColorAttachment {
        .format = mTransparencyTexture.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkAttachmentDescription colorAttachment {
        .format = mColorTexture32F.format,
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
        .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    std::array<VkAttachmentDescription, 3> attachments {
        transparentColorAttachment,
        colorAttachment,
        depthAttachment
    };

    VkAttachmentReference transparencyAttachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference transparencyInputAttachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkAttachmentReference colorAttachmentRef {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference depthAttachmentRef {
        .attachment = 2,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    uint32_t opaqueForwardSubpassPreserveAttachment = 0;
    VkSubpassDescription opaqueForwardSubpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pDepthStencilAttachment = &depthAttachmentRef,
        .preserveAttachmentCount = 1,
        .pPreserveAttachments = &opaqueForwardSubpassPreserveAttachment
    };

    std::array<uint32_t, 2> transparentFragmentCollectionSubpassPreserveAttachments {0, 1};
    VkSubpassDescription transparentFragmentCollectionSubpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .pDepthStencilAttachment = &depthAttachmentRef,
        .preserveAttachmentCount = static_cast<uint32_t>(transparentFragmentCollectionSubpassPreserveAttachments.size()),
        .pPreserveAttachments = transparentFragmentCollectionSubpassPreserveAttachments.data()
    };

    std::array<uint32_t, 2> transparencyResolutionSubpassPreserveAttachments {1, 2};
    VkSubpassDescription transparencyResolutionSubpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &transparencyAttachmentRef,
        .preserveAttachmentCount = static_cast<uint32_t>(transparencyResolutionSubpassPreserveAttachments.size()),
        .pPreserveAttachments = transparencyResolutionSubpassPreserveAttachments.data()
    };

    uint32_t blendSubpassPreserveAttachment = 2;
    VkSubpassDescription blendSubpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = 1,
        .pInputAttachments = &transparencyInputAttachmentRef,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .preserveAttachmentCount = 1,
        .pPreserveAttachments = &blendSubpassPreserveAttachment
    };

    std::array<VkSubpassDescription, 4> subpasses {
        opaqueForwardSubpass,
        transparentFragmentCollectionSubpass,
        transparencyResolutionSubpass,
        blendSubpass
    };

    std::array<VkSubpassDependency, 3> dependencies;

    dependencies.at(0).srcSubpass = 0;
    dependencies.at(0).dstSubpass = 3;
    dependencies.at(0).srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies.at(0).dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies.at(0).srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies.at(0).dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies.at(0).dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies.at(1).srcSubpass = 1;
    dependencies.at(1).dstSubpass = 2;
    dependencies.at(1).srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies.at(1).dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies.at(1).srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
    dependencies.at(1).dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies.at(1).dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies.at(2).srcSubpass = 2;
    dependencies.at(2).dstSubpass = 3;
    dependencies.at(2).srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies.at(2).dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies.at(2).srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies.at(2).dstAccessMask = VK_ACCESS_INPUT_ATTACHMENT_READ_BIT;
    dependencies.at(2).dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = static_cast<uint32_t>(subpasses.size()),
        .pSubpasses = subpasses.data(),
        .dependencyCount = static_cast<uint32_t>(dependencies.size()),
        .pDependencies = dependencies.data()
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mForwardRenderpass);
    vulkanCheck(result, "Failed to create renderpass.");
    setRenderpassDebugName(mRenderDevice, mForwardRenderpass, "Renderer::mForwardRenderpass");
}

void Renderer::createForwardFramebuffer()
{
    vkDestroyFramebuffer(mRenderDevice.device, mForwardPassFramebuffer, nullptr);

    std::array<VkImageView, 3> attachments {
        mTransparencyTexture.imageView,
        mColorTexture32F.imageView,
        mDepthTexture.imageView
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

void Renderer::createOitTransparentCollectionPipeline()
{
    PipelineSpecification specification {
        .shaderStages = {
            .vertShaderPath = "shaders/transparent_frag_collection.vert.spv",
            .fragShaderPath = "shaders/transparent_frag_collection.frag.spv"
        },
        .vertexInput = {
            .bindings = InstancedMesh::bindingDescriptions(),
            .attributes = InstancedMesh::attributeDescriptions()
        },
        .inputAssembly = {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        },
        .rasterization = {
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE,
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
        .dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        },
        .pipelineLayout = {
            .dsLayouts = {
                mCameraRenderDataDsLayout,
                mOitResourcesDsLayout,
                mMaterialsDsLayout
            }
        },
        .renderPass = mForwardRenderpass,
        .subpassIndex = 1,
        .debugName = "Renderer::mTransparentCollectionPipeline"
    };

    mTransparentCollectionPipeline = {mRenderDevice, specification};
}

void Renderer::createOitTransparencyResolutionPipeline()
{
    PipelineSpecification specification {
        .shaderStages = {
            .vertShaderPath = "shaders/fullscreen_render.vert.spv",
            .fragShaderPath = "shaders/transparency_resolution.frag.spv"
        },
        .inputAssembly = {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        },
        .rasterization = {
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE,
            .lineWidth = 1.f
        },
        .multisampling = {
            .samples = VK_SAMPLE_COUNT_1_BIT
        },
        .dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        },
        .pipelineLayout = {
            .dsLayouts = {
                mOitResourcesDsLayout
            }
        },
        .renderPass = mForwardRenderpass,
        .subpassIndex = 2,
        .debugName = "Renderer::mTransparencyResolutionPipeline"
    };

    mTransparencyResolutionPipeline = {mRenderDevice, specification};
}

void Renderer::createOpaqueForwardPassPipeline()
{
    PipelineSpecification specification {
        .shaderStages = {
            .vertShaderPath = "shaders/pbr.vert.spv",
            .fragShaderPath = "shaders/pbr.frag.spv"
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
            .samples = VK_SAMPLE_COUNT_1_BIT
        },
        .depthStencil = {
            .enableDepthTest = VK_TRUE,
            .enableDepthWrite = VK_FALSE,
            .depthCompareOp = VK_COMPARE_OP_EQUAL
        },
        .blending = {
            .enable = false
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
                mSingleImageDsLayout,
                mMaterialsDsLayout
            }
        },
        .renderPass = mForwardRenderpass,
        .subpassIndex = 0,
        .debugName = "Renderer::mOpaqueForwardPassPipeline"
    };

    mOpaqueForwardPassPipeline = VulkanGraphicsPipeline(mRenderDevice, specification);
}

void Renderer::createForwardPassBlendPipeline()
{
    PipelineSpecification specification {
        .shaderStages = {
            .vertShaderPath = "shaders/fullscreen_render.vert.spv",
            .fragShaderPath = "shaders/transparency_blend.frag.spv"
        },
        .inputAssembly = {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        },
        .rasterization = {
            .polygonMode = VK_POLYGON_MODE_FILL,
            .cullMode = VK_CULL_MODE_NONE,
            .lineWidth = 1.f
        },
        .multisampling = {
            .samples = VK_SAMPLE_COUNT_1_BIT
        },
        .blending = {
            .enable = true,
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp = VK_BLEND_OP_ADD
        },
        .dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        },
        .pipelineLayout = {
            .dsLayouts = {
                mSingleInputAttachmentDsLayout
            }
        },
        .renderPass = mForwardRenderpass,
        .subpassIndex = 3,
        .debugName = "Renderer::mForwardPassBlendPipeline"
    };

    mForwardPassBlendPipeline = {mRenderDevice, specification};
}

void Renderer::createGridRenderpass()
{
    VkAttachmentDescription colorAttachment {
        .format = mColorTexture32F.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkAttachmentDescription depthAttachment {
        .format = mDepthTexture.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
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

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mGridRenderpass);
    vulkanCheck(result, "Failed to create renderpass.");
    setRenderpassDebugName(mRenderDevice, mGridRenderpass, "Renderer::mGridRenderpass");
}

void Renderer::createGridFramebuffer()
{
    vkDestroyFramebuffer(mRenderDevice.device, mGridFramebuffer, nullptr);

    std::array<VkImageView, 2> imageViews {
        mColorTexture32F.imageView,
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
        .blending = {
            .enable = true,
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp = VK_BLEND_OP_ADD,
        },
        .dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        },
        .pipelineLayout = {
            .dsLayouts = {
                mCameraRenderDataDsLayout,
            },
            .pushConstantRange = VkPushConstantRange {
                .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
                .offset = 0,
                .size = sizeof(GridData),
            }
        },
        .renderPass = mGridRenderpass,
        .subpassIndex = 0,
        .debugName = "Renderer::mGridPipeline"
    };

    mGridPipeline = VulkanGraphicsPipeline(mRenderDevice, specification);
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

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass,
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
        .blending = {
            .enable = false
        },
        .dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        },
        .pipelineLayout = {
            .dsLayouts = {
            mSingleImageDsLayout
            }
        },
        .renderPass = mPostProcessingRenderpass,
        .subpassIndex = 0,
        .debugName = "Renderer::mPostProcessingPipeline"
    };

    mPostProcessingPipeline = VulkanGraphicsPipeline(mRenderDevice, specification);
}

void Renderer::createLightBuffers()
{
    mDirLightSSBO = {mRenderDevice, MaxDirLights * sizeof(DirectionalLight), BufferType::Storage, MemoryType::Device};
    mSpotLightSSBO = {mRenderDevice, MaxSpotLights * sizeof(SpotLight), BufferType::Storage, MemoryType::Device};
    mPointLightSSBO = {mRenderDevice, MaxPointLights * sizeof(PointLight), BufferType::Storage, MemoryType::Device};
}

void Renderer::createLightIconTextures()
{
    stbi_set_flip_vertically_on_load(true);
    LoadedImage dirLightIcon("../assets/textures/lights/directional3.png");
    LoadedImage pointLightIcon("../assets/textures/lights/lightbulb4.png");
    LoadedImage spotLightIcon("../assets/textures/lights/spotlight2.png");
    stbi_set_flip_vertically_on_load(false);

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

    VkAttachmentDescription depthAttachment {
        .format = mIconDepthTexture.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    std::array<VkAttachmentDescription, 2> attachments {colorAttachment, depthAttachment};

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

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mLightIconRenderpass);
    vulkanCheck(result, "Failed to create renderpass.");
    setRenderpassDebugName(mRenderDevice, mLightIconRenderpass, "Renderer::mLightIconRenderpass");
}

void Renderer::createLightIconFramebuffer()
{
    vkDestroyFramebuffer(mRenderDevice.device, mLightIconFramebuffer, nullptr);

    std::array<VkImageView, 2> attachments {
        mColorTexture8U.imageView,
        mIconDepthTexture.imageView
    };

    VkFramebufferCreateInfo framebufferCreateInfo {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = mLightIconRenderpass,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
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
        .blending = {
            .enable = true,
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .alphaBlendOp = VK_BLEND_OP_ADD
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
            .pushConstantRange = VkPushConstantRange {
                .stageFlags = VK_SHADER_STAGE_VERTEX_BIT,
                .offset = 0,
                .size = sizeof(glm::vec3)
            }
        },
        .renderPass = mLightIconRenderpass,
        .subpassIndex = 0,
        .debugName = "Renderer::mLightIconPipeline"
    };

    mLightIconPipeline = VulkanGraphicsPipeline(mRenderDevice, specification);
}

void Renderer::loadSkybox()
{
    VkDescriptorSetLayout dsLayout = mSingleImageDsLayout;

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = mRenderDevice.descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &dsLayout
    };

    VkResult result = vkAllocateDescriptorSets(mRenderDevice.device, &descriptorSetAllocateInfo, &mSkyboxDs);
    vulkanCheck(result, "Failed to allocate descriptor set.");

    std::array<std::string, 6> paths {
        "../assets/cubemaps/sky/Daylight_Box_Right.bmp",
        "../assets/cubemaps/sky/Daylight_Box_Left.bmp",
        "../assets/cubemaps/sky/Daylight_Box_Top.bmp",
        "../assets/cubemaps/sky/Daylight_Box_Bottom.bmp",
        "../assets/cubemaps/sky/Daylight_Box_Front.bmp",
        "../assets/cubemaps/sky/Daylight_Box_Back.bmp"
    };

    importSkybox(paths);
}
