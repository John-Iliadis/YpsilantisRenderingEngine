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

struct TextureSpecification
{
    VkFormat format;
    uint32_t width;
    uint32_t height;
    uint32_t layerCount;
    VkImageViewType imageViewType;
    VkImageUsageFlags imageUsage;
    VkImageAspectFlags imageAspect;
    VkSampleCountFlagBits samples;
    TextureWrap wrapMode;
    TextureFilter filterMode;
    bool generateMipMaps;
    VkImageCreateFlags createFlags;
};

struct VulkanSampler
{
    VkSampler sampler;
    TextureWrap wrapMode;
    TextureFilter filterMode;
};

// todo: improve
class VulkanTexture
{
public:
    VulkanImage vulkanImage;
    VulkanSampler vulkanSampler;

public:
    VulkanTexture();
    VulkanTexture(const VulkanRenderDevice& renderDevice, const TextureSpecification& specification);
    VulkanTexture(const VulkanRenderDevice& renderDevice, const TextureSpecification& specification, const void* data);
    ~VulkanTexture();

    void uploadImageData(const void* data);
    void generateMipMaps();
    void setDebugName(const std::string& debugName);

private:
    const VulkanRenderDevice* mRenderDevice;
};

uint32_t calculateMipLevels(uint32_t width, uint32_t height);

VulkanSampler createSampler(const VulkanRenderDevice& renderDevice, TextureFilter filterMode, TextureWrap wrapMode);

#endif //VULKANRENDERINGENGINE_VULKAN_TEXTURE_HPP
