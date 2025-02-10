//
// Created by Gianni on 3/01/2025.
//

#include "vulkan_texture.hpp"
#include "vulkan_buffer.hpp"

static VkDeviceSize formatSize(VkFormat format)
{
    switch (format)
    {
        case VK_FORMAT_R8G8B8A8_UNORM: return 4;
        case VK_FORMAT_R32G32B32A32_SFLOAT: return 16;
        default: assert(false);
    }
}

const char* toStr(TextureWrap wrapMode)
{
    switch (wrapMode)
    {
        case TextureWrap::Repeat: return "Repeat";
        case TextureWrap::MirroredRepeat: return "Mirrored Repeat";
        case TextureWrap::ClampToEdge: return "Clamp to Edge";
        case TextureWrap::ClampToBorder: return "Clamp to Border";
        default: return "Unknown";
    }
}

const char* toStr(TextureMagFilter magFilter)
{
    switch (magFilter)
    {
        case TextureMagFilter::Nearest: return "Nearest";
        case TextureMagFilter::Linear: return "Linear";
        default: return "Unknown";
    }
}

const char* toStr(TextureMinFilter minFilter)
{
    switch (minFilter)
    {
        case TextureMinFilter::Nearest: return "Nearest";
        case TextureMinFilter::Linear: return "Linear";
        case TextureMinFilter::NearestMipmapNearest: return "Nearest Mipmap Linear";
        case TextureMinFilter::LinearMipmapNearest: return "Linear Mipmap Nearest";
        case TextureMinFilter::NearestMipmapLinear: return "Nearest Mipmap Linear";
        case TextureMinFilter::LinearMipmapLinear: return "Linear Mipmap Linear";
        default: return "Unknown";
    }
}

// -- VulkanTexture -- //

VulkanTexture::VulkanTexture()
    : vulkanImage()
    , vulkanSampler()
    , mRenderDevice()
{
}

VulkanTexture::VulkanTexture(const VulkanRenderDevice &renderDevice, const TextureSpecification &specification)
    : vulkanImage(renderDevice,
                  specification.imageViewType,
                  specification.format,
                  specification.width, specification.height,
                  specification.imageUsage,
                  specification.imageAspect,
                  specification.generateMipMaps? calculateMipLevels(specification.width, specification.height) : 1,
                  specification.samples,
                  specification.layerCount,
                  specification.createFlags)
    , vulkanSampler(createSampler(renderDevice,
                                  specification.magFilter,
                                  specification.minFilter,
                                  specification.wrapS,
                                  specification.wrapT,
                                  specification.wrapR))
    , mRenderDevice(&renderDevice)
{
}

VulkanTexture::VulkanTexture(const VulkanRenderDevice &renderDevice, const TextureSpecification &specification, const void *data)
    : VulkanTexture(renderDevice, specification)
{
    vulkanImage.transitionLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    uploadImageData(data);
}

VulkanTexture::~VulkanTexture()
{
    if (mRenderDevice)
    {
        vkDestroySampler(mRenderDevice->device, vulkanSampler.sampler, nullptr);
    }
}

void VulkanTexture::uploadImageData(const void *data)
{
    assert(vulkanImage.layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkDeviceSize size = vulkanImage.width * vulkanImage.height * formatSize(vulkanImage.format);

    VulkanBuffer stagingBuffer(*mRenderDevice, size, BufferType::Staging, MemoryType::CPU, data);

    vulkanImage.copyBuffer(stagingBuffer);
}

void VulkanTexture::generateMipMaps()
{
    assert(vulkanImage.layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    VkCommandBuffer commandBuffer = beginSingleTimeCommands(*mRenderDevice);

    int32_t mipWidth = vulkanImage.width;
    int32_t mipHeight = vulkanImage.height;

    VkImageMemoryBarrier imageMemoryBarrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .image = vulkanImage.image,
        .subresourceRange {
            .aspectMask = vulkanImage.imageAspect,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    for (uint32_t i = 0; i < vulkanImage.mipLevels - 1; ++i)
    {
        // transition mip i + 1 to transfer dst layout
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrier.subresourceRange.baseMipLevel = i + 1;

        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             0, 0, nullptr, 0, nullptr,
                             1, &imageMemoryBarrier);

        VkImageBlit blitRegion {
            .srcSubresource {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = i,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .srcOffsets {
                {0, 0, 0},
                {mipWidth, mipHeight, 1}
            },
            .dstSubresource {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .mipLevel = i + 1,
                .baseArrayLayer = 0,
                .layerCount = 1
            },
            .dstOffsets {
                {0, 0, 0},
                {glm::max(mipWidth / 2, 1), glm::max(mipHeight / 2, 1), 1}
            }
        };

        mipWidth /= 2;
        mipHeight /= 2;

        vkCmdBlitImage(commandBuffer,
                       vulkanImage.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       vulkanImage.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1, &blitRegion,
                       VK_FILTER_LINEAR);

        // transition mip i + 1 to transfer src layout
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr,
                             1, &imageMemoryBarrier);
    }

    endSingleTimeCommands(*mRenderDevice, commandBuffer);
}

void VulkanTexture::setDebugName(const std::string &debugName)
{
    vulkanImage.setDebugName(debugName);
    setVulkanObjectDebugName(*mRenderDevice, VK_OBJECT_TYPE_SAMPLER, debugName, vulkanSampler.sampler);
}

VulkanTexture::VulkanTexture(VulkanTexture&& other) noexcept
    : VulkanTexture()
{
    swap(other);
}

VulkanTexture &VulkanTexture::operator=(VulkanTexture&& other) noexcept
{
    if (this != &other)
        swap(other);
    return *this;
}

void VulkanTexture::swap(VulkanTexture &other) noexcept
{
    std::swap(mRenderDevice, other.mRenderDevice);
    std::swap(vulkanSampler, other.vulkanSampler);
    vulkanImage.swap(other.vulkanImage);
}

uint32_t calculateMipLevels(uint32_t width, uint32_t height)
{
    return static_cast<uint32_t>(glm::floor(glm::log2((float)glm::max(width, height)))) + 1;
}

VulkanSampler createSampler(const VulkanRenderDevice& renderDevice,
                            TextureMagFilter magFilter,
                            TextureMinFilter minFilter,
                            TextureWrap wrapS,
                            TextureWrap wrapT,
                            TextureWrap wrapR)
{
    VulkanSampler sampler {
        .magFilter = magFilter,
        .minFilter = minFilter,
        .wrapS = wrapS,
        .wrapT = wrapT,
        .wrapR = wrapR
    };

    VkBool32 anisotropyEnable = VK_FALSE;
    float maxAnisotropy = 0.f;
    VkFilter vkMagFilter;
    VkFilter vkMinFilter;
    VkSamplerMipmapMode mipmapMode;
    VkSamplerAddressMode addressModeS;
    VkSamplerAddressMode addressModeT;
    VkSamplerAddressMode addressModeR;

    switch (magFilter)
    {
        case TextureMagFilter::Nearest:
            vkMagFilter = VK_FILTER_NEAREST;
            break;
        case TextureMagFilter::Linear:
            vkMagFilter = VK_FILTER_LINEAR;
            break;
    }

    switch (minFilter)
    {
        case TextureMinFilter::Nearest:
            vkMinFilter = VK_FILTER_NEAREST;
            mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            break;
        case TextureMinFilter::Linear:
            vkMinFilter = VK_FILTER_LINEAR;
            mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            break;
        case TextureMinFilter::NearestMipmapNearest:
            vkMinFilter = VK_FILTER_NEAREST;
            mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            break;
        case TextureMinFilter::LinearMipmapNearest:
            vkMinFilter = VK_FILTER_LINEAR;
            mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            break;
        case TextureMinFilter::NearestMipmapLinear:
            vkMinFilter = VK_FILTER_NEAREST;
            mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            break;
        case TextureMinFilter::LinearMipmapLinear:
            vkMinFilter = VK_FILTER_LINEAR;
            mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            break;
    }

    switch (wrapS)
    {
        case TextureWrap::Repeat: addressModeS = VK_SAMPLER_ADDRESS_MODE_REPEAT; break;
        case TextureWrap::MirroredRepeat: addressModeS = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT; break;
        case TextureWrap::ClampToEdge: addressModeS = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE; break;
        case TextureWrap::ClampToBorder: addressModeS = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER; break;
    }

    switch (wrapT)
    {
        case TextureWrap::Repeat:
            addressModeT = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            break;
        case TextureWrap::MirroredRepeat:
            addressModeT = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            break;
        case TextureWrap::ClampToEdge:
            addressModeT = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            break;
        case TextureWrap::ClampToBorder:
            addressModeT = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            break;
    }

    switch (wrapR)
    {
        case TextureWrap::Repeat:
            addressModeR = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            break;
        case TextureWrap::MirroredRepeat:
            addressModeR = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            break;
        case TextureWrap::ClampToEdge:
            addressModeR = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            break;
        case TextureWrap::ClampToBorder:
            addressModeR = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            break;
    }

    if (minFilter == TextureMinFilter::Linear ||
        minFilter == TextureMinFilter::LinearMipmapLinear ||
        minFilter == TextureMinFilter::NearestMipmapLinear)
    {
        anisotropyEnable = VK_TRUE;
        maxAnisotropy = glm::min(renderDevice.getDeviceProperties().limits.maxSamplerAnisotropy, 16.f);
    }

    VkSamplerCreateInfo samplerCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = vkMagFilter,
        .minFilter = vkMinFilter,
        .mipmapMode = mipmapMode,
        .addressModeU = addressModeS,
        .addressModeV = addressModeT,
        .addressModeW = addressModeR,
        .mipLodBias = 0.f,
        .anisotropyEnable = anisotropyEnable,
        .maxAnisotropy = maxAnisotropy,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_NEVER,
        .minLod = 0.f,
        .maxLod = VK_LOD_CLAMP_NONE,
        .unnormalizedCoordinates = VK_FALSE
    };

    VkResult result = vkCreateSampler(renderDevice.device, &samplerCreateInfo, nullptr, &sampler.sampler);
    vulkanCheck(result, "Failed to create sampler.");

    return sampler;
}
