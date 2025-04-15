//
// Created by Gianni on 24/02/2025.
//

#include "equirectangular_map_loader.hpp"

static const std::array<glm::mat4, 6> views {
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
    glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3( 0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
};

EquirectangularMapLoader::EquirectangularMapLoader(const VulkanRenderDevice &renderDevice, const std::string &path)
    : mRenderDevice(renderDevice)
    , mSuccess()
{
    float* data = stbi_loadf(path.data(), &mWidth, &mHeight, nullptr, 4);

    if (!data) return;

    mProj = glm::perspective(90.f, 1.f, 0.1f, 10.f);
    mProj[1][1] *= -1.f;

    try
    {
        createEquirectangularMap(data);
        createUBO();
        createTarget();
        createRenderpass();
        createFramebuffer();
        createDescriptors();
        createPipeline();
        allocateCommandBuffer();
        execute();
        mSuccess = true;
    }
    catch (const std::exception& e)
    {
        debugLog(std::format("Equirectangular convert fail: {}", e.what()));
    }

    stbi_image_free(data);
}

EquirectangularMapLoader::~EquirectangularMapLoader()
{
    vkFreeDescriptorSets(mRenderDevice.device, mRenderDevice.descriptorPool, 1, &mDs);
    vkDestroyFramebuffer(mRenderDevice.device, mFramebuffer, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mRenderpass, nullptr);
    vkFreeCommandBuffers(mRenderDevice.device, mRenderDevice.commandPool, 1, &mCommandBuffer);
}

bool EquirectangularMapLoader::success() const
{
    return mSuccess;
}

void EquirectangularMapLoader::get(VulkanTexture &outTexture)
{
    outTexture.swap(mTarget);
}

void EquirectangularMapLoader::createEquirectangularMap(float *data)
{
    TextureSpecification specification {
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .width = static_cast<uint32_t>(mWidth),
        .height = static_cast<uint32_t>(mHeight),
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

    mEquirectangularMap = VulkanTexture(mRenderDevice, specification, data);
    mEquirectangularMap.setDebugName("EquirectangularMapLoader::mEquirectangularMap");
}

void EquirectangularMapLoader::createUBO()
{
    mUBO = VulkanBuffer(mRenderDevice,
                        sizeof(glm::mat4) * 7,
                        BufferType::Uniform,
                        MemoryType::HostCoherent);
    mUBO.mapBufferMemory(0, sizeof(glm::mat4), glm::value_ptr(mProj));
    mUBO.mapBufferMemory(sizeof(glm::mat4), sizeof(glm::mat4) * 6, views.data());
    mUBO.setDebugName("EquirectangularMapLoader::mUBO");
}

void EquirectangularMapLoader::createTarget()
{
    mCubemapSize = mWidth / 4;

    TextureSpecification specification {
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .width = static_cast<uint32_t>(mCubemapSize),
        .height = static_cast<uint32_t>(mCubemapSize),
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

    mTarget = VulkanTexture(mRenderDevice, specification);
    mTarget.setDebugName("CubemapCreatedFromEquirectangularTexture");
}

void EquirectangularMapLoader::createRenderpass()
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

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &attachment,
        .subpassCount = 1,
        .pSubpasses = &subpass
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mRenderpass);
    vulkanCheck(result, "Failed to create renderpass.");
    setRenderpassDebugName(mRenderDevice, mRenderpass, "EquirectangularMapLoader::mRenderpass");
}

void EquirectangularMapLoader::createFramebuffer()
{
    VkFramebufferCreateInfo framebufferCreateInfo {
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = mRenderpass,
        .attachmentCount = 1,
        .pAttachments = &mTarget.imageView,
        .width = mTarget.width,
        .height = mTarget.height,
        .layers = 6
    };

    VkResult result = vkCreateFramebuffer(mRenderDevice.device, &framebufferCreateInfo, nullptr, &mFramebuffer);
    vulkanCheck(result, "Failed to create framebuffer.");
    setFramebufferDebugName(mRenderDevice, mFramebuffer, "EquirectangularMapLoader::mFramebuffer");
}

void EquirectangularMapLoader::createDescriptors()
{
    DsLayoutSpecification dsLayoutSpecification {
        .bindings = {
            binding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_GEOMETRY_BIT),
            binding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
        },
        .debugName = "EquirectangularMapLoader::mDsLayout"
    };

    mDsLayout = VulkanDsLayout(mRenderDevice, dsLayoutSpecification);

    VkDescriptorSetLayout dsLayout = mDsLayout;
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = mRenderDevice.descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &dsLayout
    };

    VkResult result = vkAllocateDescriptorSets(mRenderDevice.device, &descriptorSetAllocateInfo, &mDs);
    vulkanCheck(result, "Failed to allocate descriptor set.");

    VkDescriptorBufferInfo bufferInfo {
        .buffer = mUBO.getBuffer(),
        .offset = 0,
        .range = VK_WHOLE_SIZE
    };

    VkDescriptorImageInfo imageInfo {
        .sampler = mEquirectangularMap.vulkanSampler.sampler,
        .imageView = mEquirectangularMap.imageView,
        .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

    std::array<VkWriteDescriptorSet, 2> dsWrites{};

    dsWrites.at(0).sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    dsWrites.at(0).dstSet = mDs;
    dsWrites.at(0).dstBinding = 0;
    dsWrites.at(0).dstArrayElement = 0;
    dsWrites.at(0).descriptorCount = 1;
    dsWrites.at(0).descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    dsWrites.at(0).pBufferInfo = &bufferInfo;

    dsWrites.at(1).sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    dsWrites.at(1).dstSet = mDs;
    dsWrites.at(1).dstBinding = 1;
    dsWrites.at(1).dstArrayElement = 0;
    dsWrites.at(1).descriptorCount = 1;
    dsWrites.at(1).descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    dsWrites.at(1).pImageInfo = &imageInfo;

    vkUpdateDescriptorSets(mRenderDevice.device, dsWrites.size(), dsWrites.data(), 0, nullptr);

    setVulkanObjectDebugName(mRenderDevice,
                             VK_OBJECT_TYPE_DESCRIPTOR_SET,
                             "EquirectangularMapLoader::mDs",
                             mDs);
}

void EquirectangularMapLoader::createPipeline()
{
    PipelineSpecification specification {
        .shaderStages = {
            .vertShaderPath = "shaders/equirectangular.vert.spv",
            .geomShaderPath = "shaders/equirectangular.geom.spv",
            .fragShaderPath = "shaders/equirectangular.frag.spv"
        },
        .inputAssembly = {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        },
        .viewportState = {
//            .viewport = VkViewport {
//                .x = 0.f,
//                .y = static_cast<float>(mCubemapSize),
//                .width = static_cast<float>(mCubemapSize),
//                .height = -static_cast<float>(mCubemapSize),
//                .minDepth = 0.f,
//                .maxDepth = 1.f
//            },
            .viewport = VkViewport {
                .x = 0.f,
                .y = 0.f,
                .width = static_cast<float>(mCubemapSize),
                .height = static_cast<float>(mCubemapSize),
                .minDepth = 0.f,
                .maxDepth = 1.f
            },
            .scissor = VkRect2D {
                .offset = {.x = 0, .y = 0},
                .extent = {
                    .width = static_cast<uint32_t>(mCubemapSize),
                    .height = static_cast<uint32_t>(mCubemapSize)
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
        .pipelineLayout = {.dsLayouts = {mDsLayout}},
        .renderPass = mRenderpass,
        .subpassIndex = 0,
        .debugName = "EquirectangularMapLoader::mPipeline"
    };

    mPipeline = VulkanGraphicsPipeline(mRenderDevice, specification);
}

void EquirectangularMapLoader::allocateCommandBuffer()
{
    VkCommandBufferAllocateInfo allocateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = mRenderDevice.commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    VkResult result = vkAllocateCommandBuffers(mRenderDevice.device, &allocateInfo, &mCommandBuffer);
    vulkanCheck(result, "Failed to allocate command buffer");
}

void EquirectangularMapLoader::execute()
{
    VkRenderPassBeginInfo renderPassBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mRenderpass,
        .framebuffer = mFramebuffer,
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = {
                .width = static_cast<uint32_t>(mCubemapSize),
                .height = static_cast<uint32_t>(mCubemapSize)
            }
        },
        .clearValueCount = 0,
        .pClearValues = nullptr
    };

    VkCommandBufferBeginInfo commandBufferBeginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    VkResult result = vkBeginCommandBuffer(mCommandBuffer, &commandBufferBeginInfo);
    vulkanCheck(result, "Failed to begin command buffer.");

    vkCmdBeginRenderPass(mCommandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);
    vkCmdBindDescriptorSets(mCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline, 0, 1, &mDs, 0, nullptr);
    vkCmdDraw(mCommandBuffer, 36, 1, 0, 0);
    vkCmdEndRenderPass(mCommandBuffer);
    vkEndCommandBuffer(mCommandBuffer);

    VkFence fence = createFence(mRenderDevice, false, "EquirectangularMapLoader::execute::fence");

    VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 0,
        .commandBufferCount = 1,
        .pCommandBuffers = &mCommandBuffer,
        .signalSemaphoreCount = 0
    };

    result = vkQueueSubmit(mRenderDevice.graphicsQueue, 1, &submitInfo, fence);
    vulkanCheck(result, "Failed queue submit");

    vkWaitForFences(mRenderDevice.device, 1, &fence, VK_TRUE, UINT64_MAX);
    vkDestroyFence(mRenderDevice.device, fence, nullptr);
}
