//
// Created by Gianni on 4/01/2025.
//

#include "renderer.hpp"

Renderer::Renderer(const VulkanRenderDevice& renderDevice, SaveData& saveData)
    : mRenderDevice(renderDevice)
    , mSaveData(saveData)
    , mWidth(InitialViewportWidth)
    , mHeight(InitialViewportHeight)
    , mSamples(std::min(VK_SAMPLE_COUNT_8_BIT, getMaxSampleCount(renderDevice)))
    , mCameraUBO(renderDevice, sizeof(CameraRenderData), BufferType::Uniform, MemoryType::GPU)
{
    if (saveData.contains("viewport"))
    {
        mWidth = saveData["viewport"]["width"];
        mHeight = saveData["viewport"]["height"];
    }

    mCamera = Camera(glm::vec3(), 30.f, mWidth, mHeight);

    createTextures();
    createCameraDs();
    createDisplayTexturesDsLayout();
    createMaterialDsLayout();

    createClearRenderPass();
    createClearFramebuffer();

    createPrepassRenderpass();
    createPrepassFramebuffer();
    createPrepassPipelineLayout();
    createPrepassPipeline();

    createResolveRenderpass();
    createResolveFramebuffer();
    createMultisampledNormalDepthDsLayout();
    createResolvedNormalDepthDsLayout();
    allocateMultisampledNormalDepthDs();
    allocateResolvedNormalDepthDs();
    updateMultisampledNormalDepthDs();
    updateResolvedNormalDepthDs();
    createResolvePipelineLayout();
    createResolvePipeline();

    createColorDepthRenderpass();
    createColorDepthFramebuffer();

    createCubemapDsLayout();
    createSkyboxPipelineLayout();
    createSkyboxPipeline();

    createGridDsLayout();
    createGridPipelineLayout();
    createGridPipeline();

    createSsaoRenderpass();
    createSsaoFramebuffer();
    createSsaoPipelineLayout();
    createSsaoPipeline();
    createSsaoDsLayout();
    allocateSsaoDs();
    updateSsaoDs();

    createShadingRenderpass();
    createShadingFramebuffer();
    createShadingPipelineLayout();
    createShadingPipeline();
    createResolvedColorDsLayout();
    allocateResolvedColorDs();
    updateResolveColorDs();

    createPostProcessingRenderpass();
    createPostProcessingFramebuffer();
    createPostProcessingPipelineLayout();
    createPostProcessingPipeline();
}

Renderer::~Renderer()
{
    vkDestroyPipeline(mRenderDevice.device, mPrepassPipeline, nullptr);
    vkDestroyPipeline(mRenderDevice.device, mResolvePipeline, nullptr);
    vkDestroyPipeline(mRenderDevice.device, mSkyboxPipeline, nullptr);
    vkDestroyPipeline(mRenderDevice.device, mGridPipeline, nullptr);
    vkDestroyPipeline(mRenderDevice.device, mSsaoPipeline, nullptr);
    vkDestroyPipeline(mRenderDevice.device, mShadingPipeline, nullptr);
    vkDestroyPipeline(mRenderDevice.device, mPostProcessingPipeline, nullptr);

    vkDestroyPipelineLayout(mRenderDevice.device, mPrepassPipelineLayout, nullptr);
    vkDestroyPipelineLayout(mRenderDevice.device, mResolvePipelineLayout, nullptr);
    vkDestroyPipelineLayout(mRenderDevice.device, mSkyboxPipelineLayout, nullptr);
    vkDestroyPipelineLayout(mRenderDevice.device, mGridPipelineLayout, nullptr);
    vkDestroyPipelineLayout(mRenderDevice.device, mSsaoPipelineLayout, nullptr);
    vkDestroyPipelineLayout(mRenderDevice.device, mShadingPipelineLayout, nullptr);
    vkDestroyPipelineLayout(mRenderDevice.device, mPostProcessingPipelineLayout, nullptr);

    vkDestroyFramebuffer(mRenderDevice.device, mClearFramebuffer, nullptr);
    vkDestroyFramebuffer(mRenderDevice.device, mPrepassFramebuffer, nullptr);
    vkDestroyFramebuffer(mRenderDevice.device, mResolveFramebuffer, nullptr);
    vkDestroyFramebuffer(mRenderDevice.device, mColorDepthFramebuffer, nullptr);
    vkDestroyFramebuffer(mRenderDevice.device, mSsaoFramebuffer, nullptr);
    vkDestroyFramebuffer(mRenderDevice.device, mShadingFramebuffer, nullptr);
    vkDestroyFramebuffer(mRenderDevice.device, mPostProcessingFramebuffer, nullptr);

    vkDestroyRenderPass(mRenderDevice.device, mClearRenderpass, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mPrepassRenderpass, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mResolveRenderpass, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mColorDepthRenderpass, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mSsaoRenderpass, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mShadingRenderpass, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mPostProcessingRenderpass, nullptr);

    vkDestroyDescriptorSetLayout(mRenderDevice.device, mCameraDsLayout, nullptr);
    vkDestroyDescriptorSetLayout(mRenderDevice.device, mMultisampledNormalDepthDsLayout, nullptr);
    vkDestroyDescriptorSetLayout(mRenderDevice.device, mResolvedNormalDepthDsLayout, nullptr);
    vkDestroyDescriptorSetLayout(mRenderDevice.device, mResolvedColorDsLayout, nullptr);
    vkDestroyDescriptorSetLayout(mRenderDevice.device, mCubemapDsLayout, nullptr);
    vkDestroyDescriptorSetLayout(mRenderDevice.device, mGridDsLayout, nullptr);
    vkDestroyDescriptorSetLayout(mRenderDevice.device, mSsaoDsLayout, nullptr);
    vkDestroyDescriptorSetLayout(mRenderDevice.device, mDisplayTexturesDSLayout, nullptr);
    vkDestroyDescriptorSetLayout(mRenderDevice.device, mMaterialsDsLayout, nullptr);
}

void Renderer::render()
{
    updateCameraUBO();

    VkCommandBuffer commandBuffer = beginSingleTimeCommands(mRenderDevice);

    setViewport(commandBuffer);

    executeClearRenderpass(commandBuffer);
    executePrepass(commandBuffer);
    executeResolveRenderpass(commandBuffer);
    executeSsaoRenderpass(commandBuffer);
    executeShadingRenderpass(commandBuffer);
    executePostProcessingRenderpass(commandBuffer);

    endSingleTimeCommands(mRenderDevice, commandBuffer);
}

void Renderer::importModel(const std::filesystem::path &path)
{
    bool loaded = std::find_if(mModels.begin(), mModels.end(), [&path] (const auto& pair) {
        return path == pair.second->path;
    }) != mModels.end();

    if (loaded)
    {
        debugLog("Model is already loaded.");
        return;
    }

    EnqueueCallback callback = [this] (Task&& t) { mTaskQueue.push(std::move(t)); };
    mLoadedModelFutures.push_back(ModelImporter::loadModel(path, &mRenderDevice, callback));
}

void Renderer::deleteModel(uuid32_t id)
{
    SNS::publishMessage(Topic::Type::Renderer, Message::modelDeleted(id));
    mDeferDeleteModel = id;
}

void Renderer::resize(uint32_t width, uint32_t height)
{
    mWidth = width;
    mHeight = height;

    mCamera.resize(width, height);

    createTextures();

    createClearFramebuffer();
    createPrepassFramebuffer();
    createResolveFramebuffer();
    createColorDepthFramebuffer();
    createSsaoFramebuffer();
    createShadingFramebuffer();
    createPostProcessingFramebuffer();

    updateMultisampledNormalDepthDs();
    updateResolvedNormalDepthDs();
    updateSsaoDs();
    updateResolveColorDs();
}

void Renderer::processMainThreadTasks()
{
    while (auto task = mTaskQueue.pop())
        (*task)();

    for (auto itr = mLoadedModelFutures.begin(); itr != mLoadedModelFutures.end();)
    {
        if (itr->wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            auto model = itr->get();

            uuid32_t modelID = UUIDRegistry::generateModelID();

            mModels.emplace(modelID, model);

            model->id = modelID;
            model->createMaterialsUBO();
            model->createTextureDescriptorSets(mDisplayTexturesDSLayout);
            model->createMaterialsDescriptorSet(mMaterialsDsLayout);

            itr = mLoadedModelFutures.erase(itr);
        }
        else
            ++itr;
    }
}

void Renderer::releaseResources()
{
    if (mDeferDeleteModel)
    {
        mModels.erase(mDeferDeleteModel);
        mDeferDeleteModel = 0;
    }
}

void Renderer::executeClearRenderpass(VkCommandBuffer commandBuffer)
{
    static constexpr VkClearValue colorClear {.color = {0.2f, 0.2f, 0.2f, 1.f}};
    static constexpr VkClearValue depthClear {.depthStencil = {.depth = 1.f, .stencil = 0}};
    static constexpr VkClearValue normalClear {.color = {0.f, 0.f, 0.f, 0.f}};

    static std::array<VkClearValue, 3> clearValues {
        colorClear,
        depthClear,
        normalClear
    };

    VkRenderPassBeginInfo renderPassBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mClearRenderpass,
        .framebuffer = mClearFramebuffer,
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = {.width = mWidth, .height = mHeight}
        },
        .clearValueCount = static_cast<uint32_t>(clearValues.size()),
        .pClearValues = clearValues.data()
    };

    beginDebugLabel(commandBuffer, "Clear Renderpass");
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdEndRenderPass(commandBuffer);
    endDebugLabel(commandBuffer);
}

void Renderer::executePrepass(VkCommandBuffer commandBuffer)
{
    VkRenderPassBeginInfo renderPassBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mPrepassRenderpass,
        .framebuffer = mPrepassFramebuffer,
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = {.width = mWidth, .height = mHeight}
        },
        .clearValueCount = 0,
        .pClearValues = nullptr
    };

    std::array<VkDescriptorSet, 1> descriptorSets {
        mCameraDs
    };

    beginDebugLabel(commandBuffer, "Prepass");
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPrepassPipeline);

    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                            mPrepassPipelineLayout,
                            0, descriptorSets.size(), descriptorSets.data(),
                            0, nullptr);

    renderModels(commandBuffer, mPrepassPipelineLayout);

    vkCmdEndRenderPass(commandBuffer);
    endDebugLabel(commandBuffer);
}

void Renderer::executeResolveRenderpass(VkCommandBuffer commandBuffer)
{
    VkRenderPassBeginInfo renderPassBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mResolveRenderpass,
        .framebuffer = mResolveFramebuffer,
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = {.width = mWidth, .height = mHeight}
        },
        .clearValueCount = 0,
        .pClearValues = nullptr
    };

    beginDebugLabel(commandBuffer, "Resolve Renderpass");
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mResolvePipeline);

    vkCmdBindDescriptorSets(commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            mResolvePipelineLayout,
                            0, 1, &mMultisampledNormalDepthDs,
                            0, nullptr);

    vkCmdPushConstants(commandBuffer,
                       mResolvePipelineLayout,
                       VK_SHADER_STAGE_FRAGMENT_BIT,
                       0, sizeof(uint32_t),
                       &mSamples);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

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
        mResolvedNormalDepthDs
    };

    beginDebugLabel(commandBuffer, "SSAO Renderpass");
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mSsaoPipeline);

    vkCmdBindDescriptorSets(commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            mSsaoPipelineLayout,
                            0, descriptorSets.size(), descriptorSets.data(),
                            0, nullptr);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);
    endDebugLabel(commandBuffer);
}

void Renderer::executeShadingRenderpass(VkCommandBuffer commandBuffer)
{
    VkRenderPassBeginInfo renderPassBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mShadingRenderpass,
        .framebuffer = mShadingFramebuffer,
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = {.width = mWidth, .height = mHeight}
        },
        .clearValueCount = 0,
        .pClearValues = nullptr
    };

    std::array<VkDescriptorSet, 2> descriptorSets {
        mCameraDs,
        mSsaoDs
    };

    beginDebugLabel(commandBuffer, "Shading Renderpass");
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mShadingPipeline);

    vkCmdBindDescriptorSets(commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            mShadingPipelineLayout,
                            0, descriptorSets.size(), descriptorSets.data(),
                            0, nullptr);

    renderModels(commandBuffer, mShadingPipelineLayout);

    vkCmdEndRenderPass(commandBuffer);
    endDebugLabel(commandBuffer);
}

void Renderer::executePostProcessingRenderpass(VkCommandBuffer commandBuffer)
{
    VkRenderPassBeginInfo renderPassBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mPostProcessingRenderpass,
        .framebuffer = mPostProcessingFramebuffer,
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = {.width = mWidth, .height = mHeight}
        },
        .clearValueCount = 0,
        .pClearValues = nullptr
    };

    beginDebugLabel(commandBuffer, "Post Processing Renderpass");
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPostProcessingPipeline);

    vkCmdBindDescriptorSets(commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            mPostProcessingPipelineLayout,
                            0, 1, &mResolvedColorDs,
                            0, nullptr);

    vkCmdDraw(commandBuffer, 3, 1, 0, 0);

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
        .extent = {.width = mWidth, .height = mHeight}
    };

    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
}

void Renderer::renderModels(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout)
{
    for (const auto& [id, model] : mModels)
        model->render(commandBuffer, pipelineLayout);
}

void Renderer::createColorTextures()
{
    TextureSpecification specification {
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .width = mWidth,
        .height = mHeight,
        .layerCount = 1,
        .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                      VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                      VK_IMAGE_USAGE_SAMPLED_BIT,
        .imageAspect = VK_IMAGE_ASPECT_COLOR_BIT,
        .samples = mSamples,
        .magFilter = TextureMagFilter::Nearest,
        .minFilter = TextureMinFilter::Nearest,
        .wrapS = TextureWrap::ClampToEdge,
        .wrapT = TextureWrap::ClampToEdge,
        .generateMipMaps = false
    };

    mColorTexture = VulkanTexture(mRenderDevice, specification);
    mColorTexture.setDebugName("Renderer::mColorTexture");

    specification.samples = VK_SAMPLE_COUNT_1_BIT;
    specification.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    mResolvedColorTexture = VulkanTexture(mRenderDevice, specification);
    mResolvedColorTexture.setDebugName("Renderer::mResolvedColorTexture");
}

void Renderer::createDepthTextures()
{
    TextureSpecification specification {
        .format = VK_FORMAT_D32_SFLOAT,
        .width = mWidth,
        .height = mHeight,
        .layerCount = 1,
        .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
        .imageUsage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .imageAspect = VK_IMAGE_ASPECT_DEPTH_BIT,
        .samples = mSamples,
        .magFilter = TextureMagFilter::Nearest,
        .minFilter = TextureMinFilter::Nearest,
        .wrapS = TextureWrap::ClampToEdge,
        .wrapT = TextureWrap::ClampToEdge,
        .generateMipMaps = false
    };

    mDepthTexture = VulkanTexture(mRenderDevice, specification);
    mDepthTexture.setDebugName("Renderer::mDepthTexture");

    specification.format = VK_FORMAT_R32_SFLOAT;
    specification.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    specification.imageAspect = VK_IMAGE_ASPECT_COLOR_BIT;
    specification.samples = VK_SAMPLE_COUNT_1_BIT;
    mResolvedDepthTexture = VulkanTexture(mRenderDevice, specification);
    mResolvedDepthTexture.setDebugName("Renderer::mResolvedDepthTexture");
}

void Renderer::createNormalTextures()
{
    TextureSpecification specification {
        .format = VK_FORMAT_R16G16B16A16_SFLOAT,
        .width = mWidth,
        .height = mHeight,
        .layerCount = 1,
        .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .imageAspect = VK_IMAGE_ASPECT_COLOR_BIT,
        .samples = mSamples,
        .magFilter = TextureMagFilter::Nearest,
        .minFilter = TextureMinFilter::Nearest,
        .wrapS = TextureWrap::ClampToEdge,
        .wrapT = TextureWrap::ClampToEdge,
        .generateMipMaps = false
    };

    mNormalTexture = VulkanTexture(mRenderDevice, specification);
    mNormalTexture.setDebugName("Renderer::mNormalTexture");

    specification.samples = VK_SAMPLE_COUNT_1_BIT;
    mResolvedNormalTexture = VulkanTexture(mRenderDevice, specification);
    mResolvedNormalTexture.setDebugName("Renderer::mResolvedNormalTexture");
}

void Renderer::createSsaoTexture()
{
    TextureSpecification specification {
        .format = VK_FORMAT_R32_SFLOAT,
        .width = mWidth,
        .height = mHeight,
        .layerCount = 1,
        .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
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

void Renderer::createPostProcessTexture()
{
    TextureSpecification specification {
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .width = mWidth,
        .height = mHeight,
        .layerCount = 1,
        .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        .imageAspect = VK_IMAGE_ASPECT_COLOR_BIT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .magFilter = TextureMagFilter::Nearest,
        .minFilter = TextureMinFilter::Nearest,
        .wrapS = TextureWrap::ClampToEdge,
        .wrapT = TextureWrap::ClampToEdge,
        .generateMipMaps = false
    };

    mPostProcessTexture = VulkanTexture(mRenderDevice, specification);
    mPostProcessTexture.setDebugName("Renderer::mPostProcessTexture");
}

void Renderer::createTextures()
{
    createColorTextures();
    createDepthTextures();
    createNormalTextures();
    createSsaoTexture();
    createPostProcessTexture();
}

void Renderer::createCameraDs()
{
    // ds layout
    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
    };

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &descriptorSetLayoutBinding
    };

    VkResult result = vkCreateDescriptorSetLayout(mRenderDevice.device,
                                                  &descriptorSetLayoutCreateInfo,
                                                  nullptr,
                                                  &mCameraDsLayout);
    vulkanCheck(result, "Failed to create ds layout");
    setDsLayoutDebugName(mRenderDevice, mCameraDsLayout, "Renderer::mCameraDsLayout");

    // ds
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = mRenderDevice.descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &mCameraDsLayout
    };

    result = vkAllocateDescriptorSets(mRenderDevice.device, &descriptorSetAllocateInfo, &mCameraDs);
    vulkanCheck(result, "Failed to allocate ds.");
    setDSDebugName(mRenderDevice, mCameraDs, "Renderer::mCameraDs");

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

void Renderer::updateCameraUBO()
{
    CameraRenderData renderData(mCamera.renderData());
    mCameraUBO.update(0, sizeof(CameraRenderData), &renderData);
}

void Renderer::createDisplayTexturesDsLayout()
{
    VkDescriptorSetLayoutBinding dsLayoutBinding {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };

    VkDescriptorSetLayoutCreateInfo dsLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &dsLayoutBinding
    };

    VkResult result = vkCreateDescriptorSetLayout(mRenderDevice.device,
                                                  &dsLayoutCreateInfo,
                                                  nullptr,
                                                  &mDisplayTexturesDSLayout);
    vulkanCheck(result, "Failed to create Renderer::mDisplayTexturesDSLayout.");
    setDsLayoutDebugName(mRenderDevice, mDisplayTexturesDSLayout, "Renderer::mDisplayTexturesDSLayout");
}

void Renderer::createMaterialDsLayout()
{
    VkDescriptorSetLayoutBinding materialsBinding {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = PerModelMaxMaterialCount,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };

    VkDescriptorSetLayoutBinding texturesBinding {
        .binding = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = PerModelMaxTextureCount,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };

    VkDescriptorSetLayoutBinding bindings[2] {
        materialsBinding,
        texturesBinding
    };

    VkDescriptorBindingFlagsEXT descriptorBindingFlags[2] {
        0,
        VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT
    };

    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT descriptorSetLayoutBindingFlagsCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount = 2,
        .pBindingFlags = descriptorBindingFlags
    };

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &descriptorSetLayoutBindingFlagsCreateInfo,
        .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT,
        .bindingCount = 2,
        .pBindings = bindings
    };

    VkResult result = vkCreateDescriptorSetLayout(mRenderDevice.device,
                                                  &descriptorSetLayoutCreateInfo,
                                                  nullptr,
                                                  &mMaterialsDsLayout);
    vulkanCheck(result, "Failed to create Renderer::mMaterialsDsLayout.");
    setDsLayoutDebugName(mRenderDevice, mMaterialsDsLayout, "Renderer::mMaterialsDsLayout");
}

void Renderer::createClearRenderPass()
{
    VkAttachmentDescription colorAttachment {
        .format = mColorTexture.vulkanImage.format,
        .samples = mSamples,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentDescription depthAttachment {
        .format = mDepthTexture.vulkanImage.format,
        .samples = mSamples,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkAttachmentDescription normalAttachment {
        .format = mNormalTexture.vulkanImage.format,
        .samples = mSamples,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    std::array<VkAttachmentDescription, 3> attachments {
        colorAttachment,
        depthAttachment,
        normalAttachment
    };

    VkAttachmentReference colorAttachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference depthAttachmentRed {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference normalAttachmentRef {
        .attachment = 2,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    std::array<VkAttachmentReference, 2> multisampledColorAttachments {
        colorAttachmentRef,
        normalAttachmentRef,
    };

    VkSubpassDescription multisampledClearSubpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = static_cast<uint32_t>(multisampledColorAttachments.size()),
        .pColorAttachments = multisampledColorAttachments.data(),
        .pDepthStencilAttachment = &depthAttachmentRed
    };

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &multisampledClearSubpass
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mClearRenderpass);
    vulkanCheck(result, "Failed to create renderpass.");
    setRenderpassDebugName(mRenderDevice, mClearRenderpass, "Renderer::mClearRenderPass");
}

void Renderer::createClearFramebuffer()
{
    vkDestroyFramebuffer(mRenderDevice.device, mClearFramebuffer, nullptr);

    std::array<VkImageView, 3> imageViews {
        mColorTexture.vulkanImage.imageView,
        mDepthTexture.vulkanImage.imageView,
        mNormalTexture.vulkanImage.imageView,
    };

    VkFramebufferCreateInfo framebufferCreateInfo {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = mClearRenderpass,
        .attachmentCount = static_cast<uint32_t>(imageViews.size()),
        .pAttachments = imageViews.data(),
        .width = mWidth,
        .height = mHeight,
        .layers = 1
    };

    VkResult result = vkCreateFramebuffer(mRenderDevice.device, &framebufferCreateInfo, nullptr, &mClearFramebuffer);
    vulkanCheck(result, "Failed to create framebuffer.");
    setFramebufferDebugName(mRenderDevice, mClearFramebuffer, "Renderer::mClearFramebuffer");
}

void Renderer::createPrepassRenderpass()
{
    VkAttachmentDescription normalAttachment {
        .format = mNormalTexture.vulkanImage.format,
        .samples = mSamples,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkAttachmentDescription depthAttachment {
        .format = mDepthTexture.vulkanImage.format,
        .samples = mSamples,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
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
        .pSubpasses = &subpass,
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mPrepassRenderpass);
    vulkanCheck(result, "Failed to create renderpass.");
    setRenderpassDebugName(mRenderDevice, mPrepassRenderpass, "Renderer::mPrepassRenderpass");
}

void Renderer::createPrepassFramebuffer()
{
    vkDestroyFramebuffer(mRenderDevice.device, mPrepassFramebuffer, nullptr);

    std::array<VkImageView, 2> imageViews {
        mNormalTexture.vulkanImage.imageView,
        mDepthTexture.vulkanImage.imageView
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

void Renderer::createPrepassPipelineLayout()
{
    std::array<VkDescriptorSetLayout, 2> dsLayouts {
        mCameraDsLayout,
        mMaterialsDsLayout
    };

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(dsLayouts.size()),
        .pSetLayouts = dsLayouts.data(),
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr
    };

    VkResult result = vkCreatePipelineLayout(mRenderDevice.device,
                                             &pipelineLayoutCreateInfo,
                                             nullptr,
                                             &mPrepassPipelineLayout);
    vulkanCheck(result, "Failed to create pipeline layout.");
    setPipelineLayoutDebugName(mRenderDevice, mPrepassPipelineLayout, "Renderer::mPrepassPipelineLayout");
}

void Renderer::createPrepassPipeline()
{
    VulkanShaderModule vertShaderModule(mRenderDevice, "shaders/prepass.vert.spv");
    VulkanShaderModule fragShaderModule(mRenderDevice, "shaders/prepass.frag.spv");

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages {
        shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule),
        shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule)
    };

    auto attribDesc = InstancedMesh::attributeDescriptions();
    auto bindingDesc = InstancedMesh::bindingDescriptions();
    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = static_cast<uint32_t >(bindingDesc.size()),
        .pVertexBindingDescriptions = bindingDesc.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attribDesc.size()),
        .pVertexAttributeDescriptions = attribDesc.data(),
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO
    };

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 1.f
    };

    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = mSamples
    };

    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_TRUE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .stencilTestEnable = VK_FALSE
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState {
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachmentState
    };

    std::array<VkDynamicState, 2> dynamicStates {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = static_cast<uint32_t >(shaderStages.size()),
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInputStateCreateInfo,
        .pInputAssemblyState = &inputAssemblyStateCreateInfo,
        .pTessellationState = &tessellationStateCreateInfo,
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &rasterizationStateCreateInfo,
        .pMultisampleState = &multisampleStateCreateInfo,
        .pDepthStencilState = &depthStencilStateCreateInfo,
        .pColorBlendState = &colorBlendStateCreateInfo,
        .pDynamicState = &dynamicStateCreateInfo,
        .layout = mPrepassPipelineLayout,
        .renderPass = mPrepassRenderpass,
        .subpass = 0
    };

    VkResult result = vkCreateGraphicsPipelines(mRenderDevice.device,
                                               VK_NULL_HANDLE,
                                               1, &graphicsPipelineCreateInfo,
                                               nullptr,
                                               &mPrepassPipeline);
    vulkanCheck(result, "Failed to create pipeline.");
    setPipelineDebugName(mRenderDevice, mPrepassPipeline, "Renderer::mPrepassPipeline");
}

void Renderer::createResolveRenderpass()
{
    VkAttachmentDescription resolvedNormalAttachment {
        .format = mResolvedNormalTexture.vulkanImage.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkAttachmentDescription resolvedDepthAttachment {
        .format = mResolvedDepthTexture.vulkanImage.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    std::array<VkAttachmentDescription, 2> attachments {{
        resolvedNormalAttachment,
        resolvedDepthAttachment
    }};

    VkAttachmentReference resolvedNormalAttachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference resolvedDepthAttachmentRef {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    std::array<VkAttachmentReference, 2> colorAttachmentsRef {
        resolvedNormalAttachmentRef,
        resolvedDepthAttachmentRef
    };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = static_cast<uint32_t>(colorAttachmentsRef.size()),
        .pColorAttachments = colorAttachmentsRef.data(),
    };

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mResolveRenderpass);
    vulkanCheck(result, "Failed to create renderpass.");
    setRenderpassDebugName(mRenderDevice, mResolveRenderpass, "Renderer::mResolveRenderpass");
}

void Renderer::createResolveFramebuffer()
{
    vkDestroyFramebuffer(mRenderDevice.device, mResolveFramebuffer, nullptr);

    std::array<VkImageView, 2> imageViews {{
        mResolvedNormalTexture.vulkanImage.imageView,
        mResolvedDepthTexture.vulkanImage.imageView
    }};

    VkFramebufferCreateInfo framebufferCreateInfo {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = mResolveRenderpass,
        .attachmentCount = static_cast<uint32_t>(imageViews.size()),
        .pAttachments = imageViews.data(),
        .width = mWidth,
        .height = mHeight,
        .layers = 1
    };

    VkResult result = vkCreateFramebuffer(mRenderDevice.device, &framebufferCreateInfo, nullptr, &mResolveFramebuffer);
    vulkanCheck(result, "Failed to create framebuffer.");
    setFramebufferDebugName(mRenderDevice, mResolveFramebuffer, "Renderer::mResolveFramebuffer");
}

void Renderer::createMultisampledNormalDepthDsLayout()
{
    VkDescriptorSetLayoutBinding normalTexBinding {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };

    VkDescriptorSetLayoutBinding depthTexBinding {
        .binding = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };

    std::array<VkDescriptorSetLayoutBinding, 2> bindings {
        normalTexBinding,
        depthTexBinding
    };

    VkDescriptorSetLayoutCreateInfo dsLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };

    VkResult result = vkCreateDescriptorSetLayout(mRenderDevice.device, &dsLayoutCreateInfo, nullptr, &mMultisampledNormalDepthDsLayout);
    vulkanCheck(result, "Failed to create descriptor set layout");
    setDsLayoutDebugName(mRenderDevice, mMultisampledNormalDepthDsLayout, "Renderer::mMultisampledNormalDepthDsLayout");
}

void Renderer::createResolvedNormalDepthDsLayout()
{
    VkDescriptorSetLayoutBinding normalBinding {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };

    VkDescriptorSetLayoutBinding depthBinding {
        .binding = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };

    std::array<VkDescriptorSetLayoutBinding, 2> bindings {
        normalBinding,
        depthBinding
    };

    VkDescriptorSetLayoutCreateInfo dsLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };

    VkResult result = vkCreateDescriptorSetLayout(mRenderDevice.device, &dsLayoutCreateInfo, nullptr, &mResolvedNormalDepthDsLayout);
    vulkanCheck(result, "Failed to create descriptor set layout");
    setDsLayoutDebugName(mRenderDevice, mResolvedNormalDepthDsLayout, "Renderer::mResolvedNormalDepthDsLayout");
}

void Renderer::allocateMultisampledNormalDepthDs()
{
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = mRenderDevice.descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &mMultisampledNormalDepthDsLayout
    };

    VkResult result = vkAllocateDescriptorSets(mRenderDevice.device, &descriptorSetAllocateInfo, &mMultisampledNormalDepthDs);
    vulkanCheck(result, "Failed to allocate descriptor set.");
    setDSDebugName(mRenderDevice, mMultisampledNormalDepthDs, "Renderer::mMultisampledNormalDepthDs");
}

void Renderer::allocateResolvedNormalDepthDs()
{
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = mRenderDevice.descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &mResolvedNormalDepthDsLayout
    };

    VkResult result = vkAllocateDescriptorSets(mRenderDevice.device, &descriptorSetAllocateInfo, &mResolvedNormalDepthDs);
    vulkanCheck(result, "Failed to allocate descriptor set.");
    setDSDebugName(mRenderDevice, mResolvedNormalDepthDs, "Renderer::mResolvedNormalDepthDs");
}

void Renderer::updateMultisampledNormalDepthDs()
{
    VkDescriptorImageInfo normalImageInfo {
        .sampler = mNormalTexture.vulkanSampler.sampler,
        .imageView = mNormalTexture.vulkanImage.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkWriteDescriptorSet normalWriteDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = mMultisampledNormalDepthDs,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &normalImageInfo
    };

    VkDescriptorImageInfo depthImageInfo {
        .sampler = mDepthTexture.vulkanSampler.sampler,
        .imageView = mDepthTexture.vulkanImage.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkWriteDescriptorSet depthWriteDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = mMultisampledNormalDepthDs,
        .dstBinding = 1,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &depthImageInfo
    };

    std::array<VkWriteDescriptorSet, 2> descriptorWrites {
        normalWriteDescriptorSet,
        depthWriteDescriptorSet
    };

    vkUpdateDescriptorSets(mRenderDevice.device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void Renderer::updateResolvedNormalDepthDs()
{
    VkDescriptorImageInfo normalImageInfo {
        .sampler = mResolvedNormalTexture.vulkanSampler.sampler,
        .imageView = mResolvedNormalTexture.vulkanImage.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkWriteDescriptorSet normalWriteDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = mResolvedNormalDepthDs,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &normalImageInfo
    };

    VkDescriptorImageInfo depthImageInfo {
        .sampler = mResolvedDepthTexture.vulkanSampler.sampler,
        .imageView = mResolvedDepthTexture.vulkanImage.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkWriteDescriptorSet depthWriteDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = mResolvedNormalDepthDs,
        .dstBinding = 1,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &depthImageInfo
    };

    std::array<VkWriteDescriptorSet, 2> descriptorWrites {
        normalWriteDescriptorSet,
        depthWriteDescriptorSet
    };

    vkUpdateDescriptorSets(mRenderDevice.device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
}

void Renderer::createResolvePipelineLayout()
{
    VkPushConstantRange pushConstantRange {
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(uint32_t)
    };

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &mMultisampledNormalDepthDsLayout,
        .pushConstantRangeCount = 1,
        .pPushConstantRanges = &pushConstantRange
    };

    VkResult result = vkCreatePipelineLayout(mRenderDevice.device,
                                             &pipelineLayoutCreateInfo,
                                             nullptr,
                                             &mResolvePipelineLayout);
    vulkanCheck(result, "Failed to create pipeline layout.");
    setPipelineLayoutDebugName(mRenderDevice, mResolvePipelineLayout, "Renderer::mResolvePipelineLayout");
}

void Renderer::createResolvePipeline()
{
    VulkanShaderModule vertShaderModule(mRenderDevice, "shaders/fullscreen_render.vert.spv");
    VulkanShaderModule fragShaderModule(mRenderDevice, "shaders/color_depth_resolve.frag.spv");

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages {
        shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule),
        shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule)
    };

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr,
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO
    };

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .lineWidth = 1.f
    };

    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
    };

    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_FALSE,
        .depthWriteEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState {
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                          VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT |
                          VK_COLOR_COMPONENT_A_BIT
    };

    std::array<VkPipelineColorBlendAttachmentState, 2> colorBlendAttachments {
        colorBlendAttachmentState,
        colorBlendAttachmentState
    };

    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size()),
        .pAttachments = colorBlendAttachments.data()
    };

    std::array<VkDynamicState, 2> dynamicStates {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = static_cast<uint32_t >(shaderStages.size()),
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInputStateCreateInfo,
        .pInputAssemblyState = &inputAssemblyStateCreateInfo,
        .pTessellationState = &tessellationStateCreateInfo,
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &rasterizationStateCreateInfo,
        .pMultisampleState = &multisampleStateCreateInfo,
        .pDepthStencilState = &depthStencilStateCreateInfo,
        .pColorBlendState = &colorBlendStateCreateInfo,
        .pDynamicState = &dynamicStateCreateInfo,
        .layout = mResolvePipelineLayout,
        .renderPass = mResolveRenderpass,
        .subpass = 0
    };

    VkResult result = vkCreateGraphicsPipelines(mRenderDevice.device,
                                                VK_NULL_HANDLE,
                                                1, &graphicsPipelineCreateInfo,
                                                nullptr,
                                                &mResolvePipeline);
    vulkanCheck(result, "Failed to create pipeline.");
    setPipelineDebugName(mRenderDevice, mResolvePipeline, "Renderer::mResolvePipeline");
}

void Renderer::createColorDepthRenderpass()
{
    VkAttachmentDescription colorAttachment {
        .format = mColorTexture.vulkanImage.format,
        .samples = mSamples,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentDescription depthAttachment {
        .format = mDepthTexture.vulkanImage.format,
        .samples = mSamples,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
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
        .pSubpasses = &subpass,
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mColorDepthRenderpass);
    vulkanCheck(result, "Failed to create renderpass.");
    setRenderpassDebugName(mRenderDevice, mColorDepthRenderpass, "Renderer::mColorDepthRenderpass");
}

void Renderer::createColorDepthFramebuffer()
{
    vkDestroyFramebuffer(mRenderDevice.device, mColorDepthFramebuffer, nullptr);

    std::array<VkImageView, 2> imageViews {
        mColorTexture.vulkanImage.imageView,
        mDepthTexture.vulkanImage.imageView,
    };

    VkFramebufferCreateInfo framebufferCreateInfo {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = mColorDepthRenderpass,
        .attachmentCount = static_cast<uint32_t>(imageViews.size()),
        .pAttachments = imageViews.data(),
        .width = mWidth,
        .height = mHeight,
        .layers = 1
    };

    VkResult result = vkCreateFramebuffer(mRenderDevice.device, &framebufferCreateInfo, nullptr, &mColorDepthFramebuffer);
    vulkanCheck(result, "Failed to create framebuffer.");
    setFramebufferDebugName(mRenderDevice, mColorDepthFramebuffer, "Renderer::mColorDepthFramebuffer");
}

void Renderer::createCubemapDsLayout()
{
    VkDescriptorSetLayoutBinding cubemapBinding {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };

    VkDescriptorSetLayoutCreateInfo dsLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &cubemapBinding
    };

    VkResult result = vkCreateDescriptorSetLayout(mRenderDevice.device, &dsLayoutCreateInfo, nullptr, &mCubemapDsLayout);
    vulkanCheck(result, "Failed to create descriptor set layout");
    setDsLayoutDebugName(mRenderDevice, mCubemapDsLayout, "Renderer::mCubemapDsLayout");
}

void Renderer::createSkyboxPipelineLayout()
{
    std::array<VkDescriptorSetLayout, 2> layouts {
        mCameraDsLayout,
        mCubemapDsLayout
    };

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(layouts.size()),
        .pSetLayouts = layouts.data(),
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr
    };

    VkResult result = vkCreatePipelineLayout(mRenderDevice.device,
                                             &pipelineLayoutCreateInfo,
                                             nullptr,
                                             &mSkyboxPipelineLayout);
    vulkanCheck(result, "Failed to create pipeline layout.");
    setPipelineLayoutDebugName(mRenderDevice, mSkyboxPipelineLayout, "Renderer::mSkyboxPipelineLayout");
}

void Renderer::createSkyboxPipeline()
{
    VulkanShaderModule vertShaderModule(mRenderDevice, "shaders/skybox.vert.spv");
    VulkanShaderModule fragShaderModule(mRenderDevice, "shaders/skybox.frag.spv");

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages {
        shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule),
        shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule)
    };

    VkVertexInputBindingDescription vertexInputBindingDescription {
        .binding = 0,
        .stride = sizeof(glm::vec3),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    VkVertexInputAttributeDescription vertexInputAttributeDescription {
        .location = 0,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = 0
    };

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 1,
        .pVertexBindingDescriptions = &vertexInputBindingDescription,
        .vertexAttributeDescriptionCount = 1,
        .pVertexAttributeDescriptions = &vertexInputAttributeDescription,
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO
    };

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .lineWidth = 1.f
    };

    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = mSamples
    };

    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_FALSE,
        .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL,
        .stencilTestEnable = VK_FALSE
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState {
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                          VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT |
                          VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachmentState
    };

    std::array<VkDynamicState, 2> dynamicStates {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = static_cast<uint32_t>(shaderStages.size()),
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInputStateCreateInfo,
        .pInputAssemblyState = &inputAssemblyStateCreateInfo,
        .pTessellationState = &tessellationStateCreateInfo,
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &rasterizationStateCreateInfo,
        .pMultisampleState = &multisampleStateCreateInfo,
        .pDepthStencilState = &depthStencilStateCreateInfo,
        .pColorBlendState = &colorBlendStateCreateInfo,
        .pDynamicState = &dynamicStateCreateInfo,
        .layout = mSkyboxPipelineLayout,
        .renderPass = mColorDepthRenderpass,
        .subpass = 0
    };

    VkResult result = vkCreateGraphicsPipelines(mRenderDevice.device,
                                                VK_NULL_HANDLE,
                                                1, &graphicsPipelineCreateInfo,
                                                nullptr,
                                                &mSkyboxPipeline);
    vulkanCheck(result, "Failed to create pipeline.");
    setPipelineDebugName(mRenderDevice, mSkyboxPipeline, "Renderer::mSkyboxPipeline");
}

void Renderer::createGridDsLayout()
{
    VkDescriptorSetLayoutBinding cameraBinding {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_VERTEX_BIT
    };

    VkDescriptorSetLayoutCreateInfo dsLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &cameraBinding
    };

    VkResult result = vkCreateDescriptorSetLayout(mRenderDevice.device, &dsLayoutCreateInfo, nullptr, &mGridDsLayout);
    vulkanCheck(result, "Failed to create descriptor set layout");
    setDsLayoutDebugName(mRenderDevice, mGridDsLayout, "Renderer::mGridDsLayout");
}

void Renderer::createGridPipelineLayout()
{
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &mGridDsLayout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr
    };

    VkResult result = vkCreatePipelineLayout(mRenderDevice.device,
                                             &pipelineLayoutCreateInfo,
                                             nullptr,
                                             &mGridPipelineLayout);
    vulkanCheck(result, "Failed to create pipeline layout.");
    setPipelineLayoutDebugName(mRenderDevice, mGridPipelineLayout, "Renderer::mGridPipelineLayout");
}

void Renderer::createGridPipeline()
{
    VulkanShaderModule vertShaderModule(mRenderDevice, "shaders/grid.vert.spv");
    VulkanShaderModule fragShaderModule(mRenderDevice, "shaders/grid.frag.spv");

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages {
        shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule),
        shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule)
    };

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO
    };

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .lineWidth = 1.f
    };

    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = mSamples
    };

    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_FALSE,
        .depthCompareOp = VK_COMPARE_OP_LESS,
        .stencilTestEnable = VK_FALSE
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState {
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                          VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT |
                          VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachmentState
    };

    std::array<VkDynamicState, 2> dynamicStates {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = static_cast<uint32_t>(shaderStages.size()),
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInputStateCreateInfo,
        .pInputAssemblyState = &inputAssemblyStateCreateInfo,
        .pTessellationState = &tessellationStateCreateInfo,
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &rasterizationStateCreateInfo,
        .pMultisampleState = &multisampleStateCreateInfo,
        .pDepthStencilState = &depthStencilStateCreateInfo,
        .pColorBlendState = &colorBlendStateCreateInfo,
        .pDynamicState = &dynamicStateCreateInfo,
        .layout = mGridPipelineLayout,
        .renderPass = mColorDepthRenderpass,
        .subpass = 0
    };

    VkResult result = vkCreateGraphicsPipelines(mRenderDevice.device,
                                                VK_NULL_HANDLE,
                                                1, &graphicsPipelineCreateInfo,
                                                nullptr,
                                                &mGridPipeline);
    vulkanCheck(result, "Failed to create pipeline.");
    setPipelineDebugName(mRenderDevice, mGridPipeline, "Renderer::mGridPipeline");
}

void Renderer::createSsaoRenderpass()
{
    VkAttachmentDescription ssaoAttachment {
        .format = mSsaoTexture.vulkanImage.format,
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
        .pColorAttachments = &ssaoAttachmentRef,
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
        .pAttachments = &mSsaoTexture.vulkanImage.imageView,
        .width = mWidth,
        .height = mHeight,
        .layers = 1
    };

    VkResult result = vkCreateFramebuffer(mRenderDevice.device, &framebufferCreateInfo, nullptr, &mSsaoFramebuffer);
    vulkanCheck(result, "Failed to create framebuffer.");
    setFramebufferDebugName(mRenderDevice, mSsaoFramebuffer, "Renderer::mSsaoFramebuffer");
}

void Renderer::createSsaoPipelineLayout()
{
    std::array<VkDescriptorSetLayout, 2> dsLayouts {
        mCameraDsLayout,
        mResolvedNormalDepthDsLayout
    };

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(dsLayouts.size()),
        .pSetLayouts = dsLayouts.data(),
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr
    };

    VkResult result = vkCreatePipelineLayout(mRenderDevice.device,
                                             &pipelineLayoutCreateInfo,
                                             nullptr,
                                             &mSsaoPipelineLayout);
    vulkanCheck(result, "Failed to create pipeline layout.");
    setPipelineLayoutDebugName(mRenderDevice, mSsaoPipelineLayout, "Renderer::mSsaoPipelineLayout");
}

void Renderer::createSsaoPipeline()
{
    VulkanShaderModule vertShaderModule(mRenderDevice, "shaders/fullscreen_render.vert.spv");
    VulkanShaderModule fragShaderModule(mRenderDevice, "shaders/ssao.frag.spv");

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages {
        shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule),
        shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule)
    };

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO
    };

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .lineWidth = 1.f
    };

    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
    };

    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_FALSE,
        .depthWriteEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState {
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                          VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT |
                          VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachmentState
    };

    std::array<VkDynamicState, 2> dynamicStates {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = static_cast<uint32_t>(shaderStages.size()),
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInputStateCreateInfo,
        .pInputAssemblyState = &inputAssemblyStateCreateInfo,
        .pTessellationState = &tessellationStateCreateInfo,
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &rasterizationStateCreateInfo,
        .pMultisampleState = &multisampleStateCreateInfo,
        .pDepthStencilState = &depthStencilStateCreateInfo,
        .pColorBlendState = &colorBlendStateCreateInfo,
        .pDynamicState = &dynamicStateCreateInfo,
        .layout = mSsaoPipelineLayout,
        .renderPass = mSsaoRenderpass,
        .subpass = 0
    };

    VkResult result = vkCreateGraphicsPipelines(mRenderDevice.device,
                                                VK_NULL_HANDLE,
                                                1, &graphicsPipelineCreateInfo,
                                                nullptr,
                                                &mSsaoPipeline);
    vulkanCheck(result, "Failed to create pipeline.");
    setPipelineDebugName(mRenderDevice, mSsaoPipeline, "Renderer::mSsaoPipeline");
}

void Renderer::createSsaoDsLayout()
{
    VkDescriptorSetLayoutBinding ssaoBinding {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };

    VkDescriptorSetLayoutCreateInfo dsLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &ssaoBinding,
    };

    VkResult result = vkCreateDescriptorSetLayout(mRenderDevice.device, &dsLayoutCreateInfo, nullptr, &mSsaoDsLayout);
    vulkanCheck(result, "Failed to create descriptor set layout");
    setDsLayoutDebugName(mRenderDevice, mSsaoDsLayout, "Renderer::mSsaoDsLayout");
}

void Renderer::allocateSsaoDs()
{
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = mRenderDevice.descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &mSsaoDsLayout
    };

    VkResult result = vkAllocateDescriptorSets(mRenderDevice.device, &descriptorSetAllocateInfo, &mSsaoDs);
    vulkanCheck(result, "Failed to allocate descriptor set.");
    setDSDebugName(mRenderDevice, mSsaoDs, "Renderer::mSsaoDs");
}

void Renderer::updateSsaoDs()
{
    VkDescriptorImageInfo ssaoImageInfo {
        .sampler = mSsaoTexture.vulkanSampler.sampler,
        .imageView = mSsaoTexture.vulkanImage.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkWriteDescriptorSet ssaoWriteDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = mSsaoDs,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &ssaoImageInfo
    };

    vkUpdateDescriptorSets(mRenderDevice.device, 1, &ssaoWriteDescriptorSet, 0, nullptr);
}

void Renderer::createShadingRenderpass()
{
    VkAttachmentDescription colorAttachment {
        .format = mColorTexture.vulkanImage.format,
        .samples = mSamples,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentDescription depthAttachment {
        .format = mDepthTexture.vulkanImage.format,
        .samples = mSamples,
        .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        .finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkAttachmentDescription resolvedColorAttachment {
        .format = mResolvedColorTexture.vulkanImage.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    std::array<VkAttachmentDescription, 3> attachments {
        colorAttachment,
        depthAttachment,
        resolvedColorAttachment
    };

    VkAttachmentReference colorAttachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference depthAttachmentRef {
        .attachment = 1,
        .layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    };

    VkAttachmentReference resolvedColorAttachmentRef {
        .attachment = 2,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef,
        .pResolveAttachments = &resolvedColorAttachmentRef,
        .pDepthStencilAttachment = &depthAttachmentRef,
    };

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .subpassCount = 1,
        .pSubpasses = &subpass,
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mShadingRenderpass);
    vulkanCheck(result, "Failed to create renderpass.");
    setRenderpassDebugName(mRenderDevice, mShadingRenderpass, "Renderer::mShadingRenderpass");
}

void Renderer::createShadingFramebuffer()
{
    vkDestroyFramebuffer(mRenderDevice.device, mShadingFramebuffer, nullptr);

    std::array<VkImageView, 3> attachments {
        mColorTexture.vulkanImage.imageView,
        mDepthTexture.vulkanImage.imageView,
        mResolvedColorTexture.vulkanImage.imageView
    };

    VkFramebufferCreateInfo framebufferCreateInfo {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = mShadingRenderpass,
        .attachmentCount = static_cast<uint32_t>(attachments.size()),
        .pAttachments = attachments.data(),
        .width = mWidth,
        .height = mHeight,
        .layers = 1
    };

    VkResult result = vkCreateFramebuffer(mRenderDevice.device, &framebufferCreateInfo, nullptr, &mShadingFramebuffer);
    vulkanCheck(result, "Failed to create framebuffer.");
    setFramebufferDebugName(mRenderDevice, mShadingFramebuffer, "Renderer::mShadingFramebuffer");
}

void Renderer::createShadingPipelineLayout()
{
    std::array<VkDescriptorSetLayout, 3> dsLayouts {
        mCameraDsLayout,
        mSsaoDsLayout,
        mMaterialsDsLayout
    };

    VkPushConstantRange pushConstantRange {
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT,
        .offset = 0,
        .size = sizeof(uint32_t)
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
                                             &mShadingPipelineLayout);
    vulkanCheck(result, "Failed to create pipeline layout.");
    setPipelineLayoutDebugName(mRenderDevice, mShadingPipelineLayout, "Renderer::mShadingPipelineLayout");
}

void Renderer::createShadingPipeline()
{
    VulkanShaderModule vertShaderModule(mRenderDevice, "shaders/pbr.vert.spv");
    VulkanShaderModule fragShaderModule(mRenderDevice, "shaders/pbr.frag.spv");

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages {
        shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule),
        shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule)
    };

    auto attribDesc = InstancedMesh::attributeDescriptions();
    auto bindingDesc = InstancedMesh::bindingDescriptions();
    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = static_cast<uint32_t >(bindingDesc.size()),
        .pVertexBindingDescriptions = bindingDesc.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(attribDesc.size()),
        .pVertexAttributeDescriptions = attribDesc.data(),
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO
    };

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_BACK_BIT,
        .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
        .lineWidth = 1.f
    };

    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = mSamples
    };

    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = VK_FALSE,
        .depthCompareOp = VK_COMPARE_OP_EQUAL,
        .stencilTestEnable = VK_FALSE
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState {
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                          VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT |
                          VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachmentState
    };

    std::array<VkDynamicState, 2> dynamicStates {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = static_cast<uint32_t>(shaderStages.size()),
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInputStateCreateInfo,
        .pInputAssemblyState = &inputAssemblyStateCreateInfo,
        .pTessellationState = &tessellationStateCreateInfo,
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &rasterizationStateCreateInfo,
        .pMultisampleState = &multisampleStateCreateInfo,
        .pDepthStencilState = &depthStencilStateCreateInfo,
        .pColorBlendState = &colorBlendStateCreateInfo,
        .pDynamicState = &dynamicStateCreateInfo,
        .layout = mShadingPipelineLayout,
        .renderPass = mShadingRenderpass,
        .subpass = 0
    };

    VkResult result = vkCreateGraphicsPipelines(mRenderDevice.device,
                                                VK_NULL_HANDLE,
                                                1, &graphicsPipelineCreateInfo,
                                                nullptr,
                                                &mShadingPipeline);
    vulkanCheck(result, "Failed to create pipeline.");
    setPipelineDebugName(mRenderDevice, mShadingPipeline, "Renderer::mShadingPipeline");
}

void Renderer::createResolvedColorDsLayout()
{
    VkDescriptorSetLayoutBinding resolvedColorBinding {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };

    VkDescriptorSetLayoutCreateInfo dsLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &resolvedColorBinding,
    };

    VkResult result = vkCreateDescriptorSetLayout(mRenderDevice.device, &dsLayoutCreateInfo, nullptr, &mResolvedColorDsLayout);
    vulkanCheck(result, "Failed to create descriptor set layout");
    setDsLayoutDebugName(mRenderDevice, mResolvedColorDsLayout, "Renderer::mResolvedColorDsLayout");
}

void Renderer::allocateResolvedColorDs()
{
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = mRenderDevice.descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &mResolvedColorDsLayout
    };

    VkResult result = vkAllocateDescriptorSets(mRenderDevice.device, &descriptorSetAllocateInfo, &mResolvedColorDs);
    vulkanCheck(result, "Failed to allocate descriptor set.");
    setDSDebugName(mRenderDevice, mResolvedColorDs, "Renderer::mResolvedColorDs");
}

void Renderer::updateResolveColorDs()
{
    VkDescriptorImageInfo resolvedColorImageInfo {
        .sampler = mResolvedColorTexture.vulkanSampler.sampler,
        .imageView = mResolvedColorTexture.vulkanImage.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    VkWriteDescriptorSet resolvedColorWriteDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = mResolvedColorDs,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = &resolvedColorImageInfo
    };

    vkUpdateDescriptorSets(mRenderDevice.device, 1, &resolvedColorWriteDescriptorSet, 0, nullptr);
}

void Renderer::createPostProcessingRenderpass()
{
    VkAttachmentDescription colorAttachment {
        .format = mPostProcessTexture.vulkanImage.format,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
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
        .pAttachments = &mPostProcessTexture.vulkanImage.imageView,
        .width = mWidth,
        .height = mHeight,
        .layers = 1
    };

    VkResult result = vkCreateFramebuffer(mRenderDevice.device, &framebufferCreateInfo, nullptr, &mPostProcessingFramebuffer);
    vulkanCheck(result, "Failed to create framebuffer.");
    setFramebufferDebugName(mRenderDevice, mPostProcessingFramebuffer, "Renderer::mPostProcessingFramebuffer");
}

void Renderer::createPostProcessingPipelineLayout()
{
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &mResolvedColorDsLayout,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges = nullptr
    };

    VkResult result = vkCreatePipelineLayout(mRenderDevice.device,
                                             &pipelineLayoutCreateInfo,
                                             nullptr,
                                             &mPostProcessingPipelineLayout);
    vulkanCheck(result, "Failed to create pipeline layout.");
    setPipelineLayoutDebugName(mRenderDevice, mPostProcessingPipelineLayout, "Renderer::mPostProcessingPipelineLayout");
}

void Renderer::createPostProcessingPipeline()
{
    VulkanShaderModule vertShaderModule(mRenderDevice, "shaders/fullscreen_render.vert.spv");
    VulkanShaderModule fragShaderModule(mRenderDevice, "shaders/post_process.frag.spv");

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages {
        shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule),
        shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule)
    };

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = 0,
        .pVertexBindingDescriptions = nullptr,
        .vertexAttributeDescriptionCount = 0,
        .pVertexAttributeDescriptions = nullptr
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO
    };

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .lineWidth = 1.f
    };

    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
    };

    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_FALSE,
        .depthWriteEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState {
        .blendEnable = VK_FALSE,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                          VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT |
                          VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachmentState
    };

    std::array<VkDynamicState, 2> dynamicStates {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = static_cast<uint32_t>(shaderStages.size()),
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInputStateCreateInfo,
        .pInputAssemblyState = &inputAssemblyStateCreateInfo,
        .pTessellationState = &tessellationStateCreateInfo,
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &rasterizationStateCreateInfo,
        .pMultisampleState = &multisampleStateCreateInfo,
        .pDepthStencilState = &depthStencilStateCreateInfo,
        .pColorBlendState = &colorBlendStateCreateInfo,
        .pDynamicState = &dynamicStateCreateInfo,
        .layout = mPostProcessingPipelineLayout,
        .renderPass = mPostProcessingRenderpass,
        .subpass = 0
    };

    VkResult result = vkCreateGraphicsPipelines(mRenderDevice.device,
                                                VK_NULL_HANDLE,
                                                1, &graphicsPipelineCreateInfo,
                                                nullptr,
                                                &mPostProcessingPipeline);
    vulkanCheck(result, "Failed to create pipeline.");
    setPipelineDebugName(mRenderDevice, mPostProcessingPipeline, "Renderer::mPostProcessingPipeline");
}
