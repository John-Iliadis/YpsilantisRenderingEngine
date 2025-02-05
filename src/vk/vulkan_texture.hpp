//
// Created by Gianni on 3/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_TEXTURE_HPP
#define VULKANRENDERINGENGINE_VULKAN_TEXTURE_HPP

#include <glm/glm.hpp>
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

class VulkanTexture
{
public:
    VulkanImage vulkanImage;
    VkSampler sampler;

public:
    VulkanTexture();
    VulkanTexture(const VulkanRenderDevice& renderDevice,
                  VkImageViewType viewType,
                  VkFormat format,
                  uint32_t width, uint32_t height,
                  VkImageUsageFlags usage,
                  VkImageAspectFlags imageAspect,
                  bool generateMipMaps,
                  VkSampleCountFlagBits samples,
                  uint32_t layerCount,
                  TextureWrap textureWrap,
                  TextureFilter textureFilter,
                  VkImageCreateFlags flags = 0);
    ~VulkanTexture();

    void uploadImageData(const void* data);
    void generateMipMaps();
    void setDebugName(const std::string& debugName);

private:
    const VulkanRenderDevice* mRenderDevice;

    TextureWrap mWrapMode;
    TextureFilter mFilterMode;
};

VkSampler createSampler(const VulkanRenderDevice& renderDevice, TextureFilter filterMode, TextureWrap wrapMode);

#endif //VULKANRENDERINGENGINE_VULKAN_TEXTURE_HPP
