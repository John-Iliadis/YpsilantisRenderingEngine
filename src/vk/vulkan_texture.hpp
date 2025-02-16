//
// Created by Gianni on 3/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_TEXTURE_HPP
#define VULKANRENDERINGENGINE_VULKAN_TEXTURE_HPP

#include <glm/glm.hpp>
#include <imgui/imgui_internal.h>
#include "vulkan_image.hpp"

enum class TextureWrap
{
    Repeat,
    MirroredRepeat,
    ClampToEdge,
    ClampToBorder
};

enum class TextureMagFilter
{
    Nearest,
    Linear
};

enum class TextureMinFilter
{
    Nearest,
    Linear,
    NearestMipmapNearest,
    LinearMipmapNearest,
    NearestMipmapLinear,
    LinearMipmapLinear
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
    TextureMagFilter magFilter;
    TextureMinFilter minFilter;
    TextureWrap wrapS;
    TextureWrap wrapT;
    TextureWrap wrapR;
    bool generateMipMaps;
    VkImageCreateFlags createFlags;
};

struct VulkanSampler
{
    VkSampler sampler;
    TextureMagFilter magFilter;
    TextureMinFilter minFilter;
    TextureWrap wrapS;
    TextureWrap wrapT;
    TextureWrap wrapR;
};

const char* toStr(TextureWrap wrapMode);
const char* toStr(TextureMagFilter magFilter);
const char* toStr(TextureMinFilter minFilter);

class VulkanTexture : public VulkanImage
{
public:
    VulkanSampler vulkanSampler;

public:
    VulkanTexture();
    VulkanTexture(const VulkanRenderDevice& renderDevice, const TextureSpecification& specification);
    VulkanTexture(const VulkanRenderDevice& renderDevice, const TextureSpecification& specification, const void* data);
    ~VulkanTexture();

    VulkanTexture(VulkanTexture&& other) noexcept;
    VulkanTexture& operator=(VulkanTexture&& other) noexcept;

    void uploadImageData(const void* data, uint32_t layerIndex = 0);
    void generateMipMaps();
    void setDebugName(const std::string& debugName) override;

    void swap(VulkanTexture& other) noexcept;
};

uint32_t calculateMipLevels(uint32_t width, uint32_t height);

VulkanSampler createSampler(const VulkanRenderDevice& renderDevice,
                            TextureMagFilter magFilter,
                            TextureMinFilter minFilter,
                            TextureWrap wrapS,
                            TextureWrap wrapT,
                            TextureWrap wrapR);

#endif //VULKANRENDERINGENGINE_VULKAN_TEXTURE_HPP
