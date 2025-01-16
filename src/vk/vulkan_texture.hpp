//
// Created by Gianni on 3/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_TEXTURE_HPP
#define VULKANRENDERINGENGINE_VULKAN_TEXTURE_HPP

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include "vulkan_image.hpp"

enum class SamplerWrapMode
{
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder
};

enum class SamplerFilterMode
{
    Nearest,
    Bilinear,
    Trilinear,
    Anisotropic
};

struct VulkanTexture
{
    VulkanImage image;
    VkSampler sampler;
    uint32_t width;
    uint32_t height;
};

// texture contains linear filtering. Mip maps are generated.
VulkanTexture createTexture2D(const VulkanRenderDevice& renderDevice,
                              uint32_t width, uint32_t height,
                              uint8_t* bytes);

void generateMipMaps(const VulkanRenderDevice& renderDevice,
                     const VulkanImage& image,
                     uint32_t width, uint32_t height,
                     uint32_t mipLevels);

VkSampler createSampler(const VulkanRenderDevice& renderDevice,
                        SamplerFilterMode filterMode,
                        SamplerWrapMode wrapMode);

#endif //VULKANRENDERINGENGINE_VULKAN_TEXTURE_HPP
