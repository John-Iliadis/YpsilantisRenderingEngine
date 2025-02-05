//
// Created by Gianni on 3/01/2025.
//

#include "vulkan_image.hpp"

VulkanImage::VulkanImage()
    : mRenderDevice()
    , image()
    , imageView()
    , memory()
    , width()
    , height()
    , mipLevels()
    , layerCount()
    , format(VK_FORMAT_UNDEFINED)
    , layout(VK_IMAGE_LAYOUT_UNDEFINED)
    , imageAspect()
{
}

VulkanImage::VulkanImage(const VulkanRenderDevice &renderDevice,
                         VkImageViewType viewType,
                         VkFormat format,
                         uint32_t width, uint32_t height,
                         VkImageUsageFlags usage,
                         VkImageAspectFlags imageAspect,
                         uint32_t mipLevels,
                         VkSampleCountFlagBits samples,
                         uint32_t layerCount,
                         VkImageCreateFlags flags)
    : mRenderDevice(&renderDevice)
    , image()
    , imageView()
    , memory()
    , width(width)
    , height(height)
    , mipLevels()
    , layerCount(layerCount)
    , format(format)
    , layout(VK_IMAGE_LAYOUT_UNDEFINED)
    , imageAspect(imageAspect)
{
    VkImageCreateInfo imageCreateInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .flags = flags,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = format,
        .extent {
            .width = width,
            .height = height,
            .depth = 1
        },
        .mipLevels = mipLevels,
        .arrayLayers = layerCount,
        .samples = samples,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .initialLayout = image.layout
    };

    VkResult result = vkCreateImage(renderDevice.device, &imageCreateInfo, nullptr, &image);
    vulkanCheck(result, "Failed to create image.");

    // create image memory
    VkMemoryRequirements imageMemoryRequirements;
    vkGetImageMemoryRequirements(renderDevice.device, image, &imageMemoryRequirements);

    VkMemoryPropertyFlags memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    uint32_t imageMemoryTypeIndex = findSuitableMemoryType(renderDevice.getMemoryProperties(),
                                                           imageMemoryRequirements.memoryTypeBits,
                                                           memoryPropertyFlags).value();

    VkMemoryAllocateInfo memoryAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .allocationSize = imageMemoryRequirements.size,
        .memoryTypeIndex = imageMemoryTypeIndex
    };

    result = vkAllocateMemory(renderDevice.device, &memoryAllocateInfo, nullptr, &memory);
    vulkanCheck(result, "Failed to allocate image memory.");

    vkBindImageMemory(renderDevice.device, image, memory, 0);

    // create image view
    createImageView(viewType, imageAspect);
}

VulkanImage::~VulkanImage()
{
    destroy();
}

VulkanImage::VulkanImage(VulkanImage &&other) noexcept
    : VulkanImage()
{
    swap(other);
}

VulkanImage &VulkanImage::operator=(VulkanImage &&other) noexcept
{
    if (this != &other)
    {
        destroy();
        swap(other);
    }

    return *this;
}

void VulkanImage::swap(VulkanImage &other) noexcept
{
    std::swap(image, other.image);
    std::swap(imageView, other.imageView);
    std::swap(memory, other.memory);
    std::swap(width, other.width);
    std::swap(height, other.height);
    std::swap(mipLevels, other.mipLevels);
    std::swap(layerCount, other.layerCount);
    std::swap(format, other.format);
    std::swap(layout, other.layout);
    std::swap(imageAspect, other.imageAspect);
}

void VulkanImage::destroy()
{
    vkDestroyImageView(mRenderDevice->device, imageView, nullptr);
    vkDestroyImage(mRenderDevice->device, image, nullptr);
    vkFreeMemory(mRenderDevice->device, memory, nullptr);

    width = 0;
    height = 0;
    mipLevels = 0;
    layerCount = 0;

    format = VK_FORMAT_UNDEFINED;
    layout = VK_IMAGE_LAYOUT_UNDEFINED;
}

void VulkanImage::createImageView(VkImageViewType viewType, VkImageAspectFlags aspectMask)
{
    VkImageViewCreateInfo imageViewCreateInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = viewType,
        .format = format,
        .components = {},
        .subresourceRange {
            .aspectMask = aspectMask,
            .baseMipLevel = 0,
            .levelCount = mipLevels,
            .baseArrayLayer = 0,
            .layerCount = layerCount
        }
    };

    VkResult result = vkCreateImageView(mRenderDevice->device, &imageViewCreateInfo, nullptr, &imageView);
    vulkanCheck(result, "Failed to create image view.");
}

void VulkanImage::transitionLayout(VkImageLayout newLayout)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(*mRenderDevice);

    VkImageMemoryBarrier imageMemoryBarrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .oldLayout = layout,
        .newLayout = newLayout,
        .image = image,
        .subresourceRange {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = mipLevels, // transition all mip levels
            .baseArrayLayer = 0,
            .layerCount = layerCount
        }
    };

    VkPipelineStageFlags srcStageMask {};
    VkPipelineStageFlags dstStageMask {};

    if (layout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        imageMemoryBarrier.srcAccessMask = 0;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (layout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
    {
        imageMemoryBarrier.srcAccessMask = 0;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
    {
        imageMemoryBarrier.srcAccessMask = 0;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        srcStageMask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }
    else if (layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
    {
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = 0;
        srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    }
    else if (layout == VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        srcStageMask = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
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

    layout = newLayout;

    endSingleTimeCommands(*mRenderDevice, commandBuffer);
}

void VulkanImage::copyBuffer(const VulkanBuffer &buffer)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(*mRenderDevice);

    VkBufferImageCopy copyRegion {
        .bufferOffset = 0,
        .bufferRowLength = width,
        .bufferImageHeight = height,
        .imageSubresource {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .imageOffset {0, 0, 0},
        .imageExtent {
            .width = width,
            .height = height,
            .depth = 1
        }
    };

    vkCmdCopyBufferToImage(commandBuffer,
                           buffer.getBuffer(),
                           image.image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &copyRegion);

    endSingleTimeCommands(*mRenderDevice, commandBuffer);
}

void VulkanImage::setDebugName(const std::string &debugName)
{
    setVulkanObjectDebugName(*mRenderDevice, VK_OBJECT_TYPE_IMAGE, debugName, image);
    setVulkanObjectDebugName(*mRenderDevice, VK_OBJECT_TYPE_IMAGE_VIEW, debugName, imageView);
}

void VulkanImage::resolveImage(const VulkanImage &multisampledImage)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(*mRenderDevice);

    VkImageResolve resolve {
        .srcSubresource {
            .aspectMask = multisampledImage.imageAspect,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .srcOffset {.x = 0, .y = 0, .z = 0},
        .dstSubresource {
            .aspectMask = imageAspect,
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
                      multisampledImage.image,
                      VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                      image,
                      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                      1, &resolve);

    endSingleTimeCommands(*mRenderDevice, commandBuffer);
}
