//
// Created by Gianni on 3/01/2025.
//

#include "vulkan_texture.hpp"

static VkDeviceSize formatSize(VkFormat format)
{
    switch (format)
    {
        case VK_FORMAT_R8G8B8A8_UNORM:
            return 32;
        case VK_FORMAT_R16G16B16A16_SFLOAT:
            return 64;
        default:
            assert(false);
    }
}

VulkanTexture createTexture2D(const VulkanRenderDevice& renderDevice,
                              uint32_t width, uint32_t height,
                              VkFormat format,
                              VkImageUsageFlags usage,
                              VkImageAspectFlags imageAspect,
                              TextureWrap wrapMode,
                              TextureFilter filterMode,
                              bool enableMipsMaps)
{
    VulkanTexture texture;

    uint32_t mipLevels = 1;
    if (enableMipsMaps)
        mipLevels = static_cast<uint32_t>(glm::floor(glm::log2((float)glm::max(width, height)))) + 1;

    texture.image = createImage2D(renderDevice,
                                  format,
                                  width, height,
                                  usage,
                                  imageAspect,
                                  mipLevels);
    texture.sampler = createSampler(renderDevice, filterMode, wrapMode);

    return texture;
}

void uploadTextureData(const VulkanRenderDevice& renderDevice,
                       VulkanTexture& texture,
                       const void* data)
{
    assert(texture.image.layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

    VkDeviceSize size = texture.image.width * texture.image.height * formatSize(texture.image.format);

    VulkanBuffer stagingBuffer = createBuffer(renderDevice,
                                              size,
                                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    mapBufferMemory(renderDevice, stagingBuffer, 0, size, data);

    copyBufferToImage(renderDevice, stagingBuffer, texture.image);

    destroyBuffer(renderDevice, stagingBuffer);
}

void generateMipMaps(const VulkanRenderDevice& renderDevice, const VulkanImage& image)
{
    assert(image.layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    VkCommandBuffer commandBuffer = beginSingleTimeCommands(renderDevice.device, renderDevice.commandPool);

    int32_t mipWidth = image.width;
    int32_t mipHeight = image.height;

    VkImageMemoryBarrier imageMemoryBarrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .image = image.image,
        .subresourceRange {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    for (uint32_t i = 0; i < image.mipLevels - 1; ++i)
    {
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        imageMemoryBarrier.subresourceRange.baseMipLevel = i;

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
                       image.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1, &blitRegion,
                       VK_FILTER_LINEAR);

        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        imageMemoryBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        imageMemoryBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        vkCmdPipelineBarrier(commandBuffer,
                             VK_PIPELINE_STAGE_TRANSFER_BIT,
                             VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                             0, 0, nullptr, 0, nullptr,
                             1, &imageMemoryBarrier);
    }

    endSingleTimeCommands(renderDevice.device, renderDevice.commandPool, commandBuffer, renderDevice.graphicsQueue);
}

VkSampler createSampler(const VulkanRenderDevice& renderDevice,
                        TextureFilter filterMode,
                        TextureWrap wrapMode)
{
    VkSampler sampler;

    VkBool32 anisotropyEnable = VK_FALSE;
    float maxAnisotropy = 0.f;
    VkFilter filter;
    VkSamplerMipmapMode mipmapMode;
    VkSamplerAddressMode addressMode;

    switch (filterMode)
    {
        case TextureFilter::Nearest:
            filter = VK_FILTER_NEAREST;
            mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            break;
        case TextureFilter::Bilinear:
            filter = VK_FILTER_LINEAR;
            mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            break;
        case TextureFilter::Trilinear:
            filter = VK_FILTER_LINEAR;
            mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            break;
        case TextureFilter::Anisotropic:
            filter = VK_FILTER_LINEAR;
            mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            anisotropyEnable = VK_TRUE;
            maxAnisotropy = glm::max(16.f, renderDevice.properties.limits.maxSamplerAnisotropy);
            break;
    }

    switch (wrapMode)
    {
        case TextureWrap::Repeat:
            addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            break;
        case TextureWrap::MirroredRepeat:
            addressMode = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            break;
        case TextureWrap::ClampToEdge:
            addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            break;
        case TextureWrap::ClampToBorder:
            addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            break;
    }

    VkSamplerCreateInfo samplerCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
        .magFilter = filter,
        .minFilter = filter,
        .mipmapMode = mipmapMode,
        .addressModeU = addressMode,
        .addressModeV = addressMode,
        .addressModeW = addressMode,
        .mipLodBias = 0.f,
        .anisotropyEnable = anisotropyEnable,
        .maxAnisotropy = maxAnisotropy,
        .compareEnable = VK_FALSE,
        .compareOp = VK_COMPARE_OP_NEVER,
        .minLod = 0.f,
        .maxLod = VK_LOD_CLAMP_NONE,
        .unnormalizedCoordinates = VK_FALSE
    };

    VkResult result = vkCreateSampler(renderDevice.device, &samplerCreateInfo, nullptr, &sampler);
    vulkanCheck(result, "Failed to create sampler.");

    return sampler;
}
