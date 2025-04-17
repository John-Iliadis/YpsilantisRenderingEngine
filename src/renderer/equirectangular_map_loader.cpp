//
// Created by Gianni on 24/02/2025.
//

#include "equirectangular_map_loader.hpp"

extern const std::array<glm::mat4, 6> gViews;

EquirectangularMapLoader::EquirectangularMapLoader(const VulkanRenderDevice &renderDevice, const std::string &path)
    : mRenderDevice(renderDevice)
    , mSuccess()
{
    float* data = stbi_loadf(path.data(), &mWidth, &mHeight, nullptr, 4);

    if (!data) return;

    try
    {
        createEquirectangularMap(data);
        createUBO();
        createTarget();
        createRenderpass();
        createFramebuffer();
        createDescriptors();
        createPipeline();
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
                        sizeof(gViews),
                        BufferType::Uniform,
                        MemoryType::HostCoherent);
    mUBO.mapBufferMemory(0, sizeof(gViews), gViews.data());
    mUBO.setDebugName("EquirectangularMapLoader::mUBO");
}

void EquirectangularMapLoader::createTarget()
{
    mCubemapSize = mHeight;

    TextureSpecification specification {
        .format = VK_FORMAT_R32G32B32A32_SFLOAT,
        .width = static_cast<uint32_t>(mCubemapSize),
        .height = static_cast<uint32_t>(mCubemapSize),
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

    mTarget = VulkanTexture(mRenderDevice, specification);
    mTarget.createMipLevelImageViews(VK_IMAGE_VIEW_TYPE_CUBE);
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
        .pAttachments = &mTarget.mipLevelImageViews.at(0),
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
            .vertShaderPath = "shaders/quad.vert.spv",
            .geomShaderPath = "shaders/equirectangular.geom.spv",
            .fragShaderPath = "shaders/equirectangular.frag.spv"
        },
        .inputAssembly = {
            .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
        },
        .viewportState = {
            .viewport = VkViewport {
                .x = 0.f,
                .y = static_cast<float>(mCubemapSize),
                .width = static_cast<float>(mCubemapSize),
                .height = -static_cast<float>(mCubemapSize),
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

    VkCommandBuffer commandBuffer = beginSingleTimeCommands(mRenderDevice);

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mPipeline, 0, 1, &mDs, 0, nullptr);
    vkCmdDraw(commandBuffer, 6, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);

    mTarget.transitionLayout(commandBuffer,
                             VK_IMAGE_LAYOUT_UNDEFINED,
                             VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                             VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0,
                             VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT);
    mTarget.generateMipMaps(commandBuffer);
    mTarget.transitionLayout(commandBuffer,
                             VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT,
                             VK_ACCESS_SHADER_READ_BIT);

    endSingleTimeCommands(mRenderDevice, commandBuffer);
}
