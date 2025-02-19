//
// Created by Gianni on 3/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_IMAGE_HPP
#define VULKANRENDERINGENGINE_VULKAN_IMAGE_HPP

#include "vulkan_render_device.hpp"

class VulkanBuffer;

class VulkanImage
{
public:
    VkImage image;
    VkImageView imageView;
    VkDeviceMemory memory;

    uint32_t width;
    uint32_t height;
    uint32_t mipLevels;
    uint32_t layerCount;

    VkFormat format;
    VkImageAspectFlags imageAspect;

public:
    VulkanImage();
    VulkanImage(const VulkanRenderDevice& renderDevice,
                VkImageViewType viewType,
                VkFormat format,
                uint32_t width, uint32_t height,
                VkImageUsageFlags usage,
                VkImageAspectFlags imageAspect,
                uint32_t mipLevels = 1,
                VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT,
                uint32_t layerCount = 1,
                VkImageCreateFlags flags = 0);
    virtual ~VulkanImage();

    VulkanImage(const VulkanImage&) = delete;
    VulkanImage& operator=(const VulkanImage&) = delete;

    VulkanImage(VulkanImage&& other) noexcept;
    VulkanImage& operator=(VulkanImage&& other) noexcept;

    void transitionLayout(VkCommandBuffer commandBuffer,
                          VkImageLayout oldLayout, VkImageLayout newLayout,
                          VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
                          VkAccessFlags srcAccess, VkAccessFlags dstAccess);
    void transitionLayout(VkImageLayout oldLayout, VkImageLayout newLayout,
                          VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
                          VkAccessFlags srcAccess, VkAccessFlags dstAccess);
    void copyBuffer(const VulkanBuffer& buffer, uint32_t layerIndex = 0);
    void resolveImage(const VulkanImage& multisampledImage);
    void swap(VulkanImage& other) noexcept;
    virtual void setDebugName(const std::string& debugName);

protected:
    const VulkanRenderDevice* mRenderDevice;
};

VkImageView createImageView(const VulkanRenderDevice& renderDevice,
                            VkImage image,
                            VkImageViewType imageViewType,
                            VkFormat format,
                            VkImageAspectFlags aspectFlags,
                            uint32_t mipLevels = 1,
                            uint32_t layerIndex = 0,
                            uint32_t layerCount = 1);

#endif //VULKANRENDERINGENGINE_VULKAN_IMAGE_HPP
