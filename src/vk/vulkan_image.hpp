//
// Created by Gianni on 3/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_IMAGE_HPP
#define VULKANRENDERINGENGINE_VULKAN_IMAGE_HPP

#include <vulkan/vulkan.h>
#include "vulkan_render_device.hpp"
#include "vulkan_utils.hpp"
#include "vulkan_buffer.hpp"

struct VulkanImage
{
    VkImage image;
    VkImageView imageView;
    VkDeviceMemory memory;
    VkFormat format;
    VkImageLayout layout;
    uint32_t mipLevels;
};

VulkanImage createImage(const VulkanRenderDevice& renderDevice,
                        VkImageViewType viewType,
                        VkFormat format,
                        uint32_t width, uint32_t height,
                        VkImageUsageFlags usage,
                        VkImageAspectFlags aspectMask,
                        uint32_t mipLevels = 1,
                        VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT,
                        uint32_t layerCount = 1,
                        VkImageCreateFlags flags = 0);

VulkanImage createImage2D(const VulkanRenderDevice& renderDevice,
                          VkFormat format,
                          uint32_t width, uint32_t height,
                          VkImageUsageFlags usage,
                          VkImageAspectFlags aspectMask,
                          uint32_t mipLevels = 1,
                          VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);

VulkanImage createImageCube(const VulkanRenderDevice& renderDevice,
                            VkFormat format,
                            uint32_t width, uint32_t height,
                            VkImageUsageFlags usage,
                            VkImageAspectFlags aspectMask,
                            uint32_t mipLevels = 1,
                            VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT);

void destroyImage(const VulkanRenderDevice& renderDevice, VulkanImage& image);

VkImageView createImageView(const VulkanRenderDevice& renderDevice,
                            VkImage image,
                            VkImageViewType viewType,
                            VkFormat format,
                            VkImageAspectFlags aspectFlags,
                            uint32_t mipLevels = 1,
                            uint32_t layerCount = 1);

void transitionImageLayout(const VulkanRenderDevice& renderDevice,
                           VulkanImage& image,
                           VkImageLayout newLayout,
                           uint32_t mipLevels = 1);

void copyBufferToImage(const VulkanRenderDevice& renderDevice,
                       VulkanBuffer& buffer,
                       VulkanImage& image,
                       uint32_t width, uint32_t height);

void resolveImage(const VulkanRenderDevice& renderDevice,
                  VkImage srcImage,
                  VkImage dstImage,
                  VkImageAspectFlags aspectMask,
                  uint32_t width, uint32_t height);

void setImageDebugName(const VulkanRenderDevice& renderDevice,
                       const VulkanImage& image,
                       const char* name,
                       std::optional<uint32_t> index = {});

#endif //VULKANRENDERINGENGINE_VULKAN_IMAGE_HPP
