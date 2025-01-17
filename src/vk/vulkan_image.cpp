//
// Created by Gianni on 3/01/2025.
//

#include "vulkan_image.hpp"

VulkanImage createImage(const VulkanRenderDevice& renderDevice,
                        VkImageViewType viewType,
                        VkFormat format,
                        uint32_t width, uint32_t height,
                        VkImageUsageFlags usage,
                        VkImageAspectFlags aspectMask,
                        uint32_t mipLevels,
                        VkSampleCountFlagBits samples,
                        uint32_t layerCount,
                        VkImageCreateFlags flags)
{
    VulkanImage image {
        .width = width,
        .height = height,
        .format = format,
        .layout = VK_IMAGE_LAYOUT_UNDEFINED,
        .mipLevels = mipLevels
    };

    // create image
    VkImageCreateInfo imageCreateInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = flags,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = image.format,
        .extent {
            .width = image.width,
            .height = image.height,
            .depth = 1
        },
        .mipLevels = image.mipLevels,
        .arrayLayers = layerCount,
        .samples = samples,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = image.layout
    };

    VkResult result = vkCreateImage(renderDevice.device, &imageCreateInfo, nullptr, &image.image);
    vulkanCheck(result, "Failed to create image.");

    // create image memory
    VkMemoryRequirements imageMemoryRequirements;
    vkGetImageMemoryRequirements(renderDevice.device, image.image, &imageMemoryRequirements);

    VkMemoryPropertyFlags memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    uint32_t imageMemoryTypeIndex = findSuitableMemoryType(renderDevice.memoryProperties,
                                                           imageMemoryRequirements.memoryTypeBits,
                                                           memoryPropertyFlags).value();

    VkMemoryAllocateInfo memoryAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = imageMemoryRequirements.size,
        .memoryTypeIndex = imageMemoryTypeIndex
    };

    result = vkAllocateMemory(renderDevice.device, &memoryAllocateInfo, nullptr, &image.memory);
    vulkanCheck(result, "Failed to allocate image memory.");

    vkBindImageMemory(renderDevice.device, image.image, image.memory, 0);

    // create image view
    image.imageView = createImageView(renderDevice, image.image, viewType, image.format, aspectMask, image.mipLevels);

    return image;
}

VulkanImage createImage2D(const VulkanRenderDevice& renderDevice,
                          VkFormat format,
                          uint32_t width, uint32_t height,
                          VkImageUsageFlags usage,
                          VkImageAspectFlags aspectMask,
                          uint32_t mipLevels,
                          VkSampleCountFlagBits samples)
{
    return createImage(renderDevice,
                       VK_IMAGE_VIEW_TYPE_2D,
                       format,
                       width, height,
                       usage,
                       aspectMask,
                       mipLevels,
                       samples,
                       1);
}

VulkanImage createImageCube(const VulkanRenderDevice& renderDevice,
                            VkFormat format,
                            uint32_t width, uint32_t height,
                            VkImageUsageFlags usage,
                            VkImageAspectFlags aspectMask,
                            uint32_t mipLevels,
                            VkSampleCountFlagBits samples)
{
    return createImage(renderDevice,
                       VK_IMAGE_VIEW_TYPE_CUBE,
                       format,
                       width, height,
                       usage,
                       aspectMask,
                       mipLevels,
                       samples,
                       6,
                       VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);
}

void destroyImage(const VulkanRenderDevice& renderDevice, VulkanImage& image)
{
    vkDestroyImageView(renderDevice.device, image.imageView, nullptr);
    vkDestroyImage(renderDevice.device, image.image, nullptr);
    vkFreeMemory(renderDevice.device, image.memory, nullptr);
}

VkImageView createImageView(const VulkanRenderDevice& renderDevice,
                            VkImage image,
                            VkImageViewType viewType,
                            VkFormat format,
                            VkImageAspectFlags aspectFlags,
                            uint32_t mipLevels,
                            uint32_t layerCount)
{
    VkImageView imageView;

    VkImageViewCreateInfo imageViewCreateInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = viewType,
        .format = format,
        .components = {},
        .subresourceRange {
            .aspectMask = aspectFlags,
            .baseMipLevel = 0,
            .levelCount = mipLevels,
            .baseArrayLayer = 0,
            .layerCount = layerCount
        }
    };

    VkResult result = vkCreateImageView(renderDevice.device, &imageViewCreateInfo, nullptr, &imageView);
    vulkanCheck(result, "Failed to create image view.");

    return imageView;
}

void transitionImageLayout(const VulkanRenderDevice& renderDevice,
                           VulkanImage& image,
                           VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(renderDevice.device, renderDevice.commandPool);

    VkImageMemoryBarrier imageMemoryBarrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = image.layout,
        .newLayout = newLayout,
        .image = image.image,
        .subresourceRange {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = image.mipLevels, // transition all mip levels
            .baseArrayLayer = 0,
            .layerCount = 1
        }
    };

    VkPipelineStageFlags srcStageMask {};
    VkPipelineStageFlags dstStageMask {};

    if (image.layout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        imageMemoryBarrier.srcAccessMask = 0;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (image.layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (image.layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
    {
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = 0;
        srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    }
    else
    {
        check(false, "Operation not supported yet.");
    }

    vkCmdPipelineBarrier(commandBuffer,
                         srcStageMask,
                         dstStageMask,
                         0, 0, nullptr, 0, nullptr,
                         1, &imageMemoryBarrier);

    image.layout = newLayout;

    endSingleTimeCommands(renderDevice.device, renderDevice.commandPool, commandBuffer, renderDevice.graphicsQueue);
}

void copyBufferToImage(const VulkanRenderDevice& renderDevice,
                       const VulkanBuffer& buffer,
                       const VulkanImage& image)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(renderDevice.device, renderDevice.commandPool);

    VkBufferImageCopy copyRegion {
        .bufferOffset = 0,
        .bufferRowLength = image.width,
        .bufferImageHeight = image.height,
        .imageSubresource {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .imageOffset {0, 0, 0},
        .imageExtent {
            .width = image.width,
            .height = image.height,
            .depth = 1
        }
    };

    vkCmdCopyBufferToImage(commandBuffer,
                           buffer.buffer,
                           image.image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &copyRegion);

    endSingleTimeCommands(renderDevice.device, renderDevice.commandPool, commandBuffer, renderDevice.graphicsQueue);
}

void resolveImage(const VulkanRenderDevice& renderDevice,
                  VkImage srcImage,
                  VkImage dstImage,
                  VkImageAspectFlags aspectMask,
                  uint32_t width, uint32_t height)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(renderDevice.device, renderDevice.commandPool);

    VkImageResolve resolve {
        .srcSubresource {
            .aspectMask = aspectMask,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .srcOffset {.x = 0, .y = 0, .z = 0},
        .dstSubresource {
            .aspectMask = aspectMask,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .dstOffset {.x = 0, .y = 0, .z = 0},
        .extent {
            .width = width,
            .height = height,
            .depth = 1
        }
    };

    vkCmdResolveImage(commandBuffer,
                      srcImage,
                      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                      dstImage,
                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                      1, &resolve);

    endSingleTimeCommands(renderDevice.device, renderDevice.commandPool, commandBuffer, renderDevice.graphicsQueue);
}

void setImageDebugName(const VulkanRenderDevice& renderDevice,
                       const VulkanImage& image,
                       const char* name,
                       std::optional<uint32_t> index)
{
    setDebugVulkanObjectName(renderDevice.device,
                             VK_OBJECT_TYPE_IMAGE,
                             std::format("{}.image{}", name,index.has_value()? std::format(" [{}]", index.value()) : ""),
                             image.image);

    setDebugVulkanObjectName(renderDevice.device,
                             VK_OBJECT_TYPE_IMAGE_VIEW,
                             std::format("{}.imageView{}", name, index.has_value()? std::format(" [{}]", index.value()) : ""),
                             image.imageView);
}
