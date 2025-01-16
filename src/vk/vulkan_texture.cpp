//
// Created by Gianni on 3/01/2025.
//

#include "vulkan_texture.hpp"

VkSampler createSampler(const VulkanRenderDevice& renderDevice,
                        SamplerFilterMode filterMode,
                        SamplerWrapMode wrapMode)
{
    VkSampler sampler;

    VkBool32 anisotropyEnable = VK_FALSE;
    float maxAnisotropy = 0.f;
    VkFilter filter;
    VkSamplerMipmapMode mipmapMode;
    VkSamplerAddressMode addressMode;

    switch (filterMode)
    {
        case SamplerFilterMode::Nearest:
            filter = VK_FILTER_NEAREST;
            mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            break;
        case SamplerFilterMode::Bilinear:
            filter = VK_FILTER_LINEAR;
            mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
            break;
        case SamplerFilterMode::Trilinear:
            filter = VK_FILTER_LINEAR;
            mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            break;
        case SamplerFilterMode::Anisotropic:
            filter = VK_FILTER_LINEAR;
            mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            anisotropyEnable = VK_TRUE;
            maxAnisotropy = glm::max(16.f, renderDevice.properties.limits.maxSamplerAnisotropy);
            break;
    }

    switch (wrapMode)
    {
        case SamplerWrapMode::Repeat:
            addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            break;
        case SamplerWrapMode::MirroredRepeat:
            addressMode = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
            break;
        case SamplerWrapMode::ClampToEdge:
            addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            break;
        case SamplerWrapMode::ClampToBorder:
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
