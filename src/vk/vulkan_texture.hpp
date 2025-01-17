//
// Created by Gianni on 3/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_TEXTURE_HPP
#define VULKANRENDERINGENGINE_VULKAN_TEXTURE_HPP

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include "vulkan_image.hpp"

enum class TextureWrap
{
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder
};

enum class TextureFilter
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
};

VulkanTexture createTexture2D(const VulkanRenderDevice& renderDevice,
                              uint32_t width, uint32_t height,
                              VkFormat format,
                              VkImageUsageFlags usage,
                              VkImageAspectFlags imageAspect,
                              TextureWrap wrapMode,
                              TextureFilter filterMode,
                              bool enableMipsMaps = false);

void uploadTextureData(const VulkanRenderDevice& renderDevice,
                       VulkanTexture& texture,
                       const void* data);

void generateMipMaps(const VulkanRenderDevice& renderDevice, const VulkanImage& image);

VkSampler createSampler(const VulkanRenderDevice& renderDevice,
                        TextureFilter filterMode,
                        TextureWrap wrapMode);

#endif //VULKANRENDERINGENGINE_VULKAN_TEXTURE_HPP
