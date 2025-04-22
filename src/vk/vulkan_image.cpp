//
// Created by Gianni on 3/01/2025.
//

#include "vulkan_image.hpp"
#include "vulkan_utils.hpp"
#include "vulkan_buffer.hpp"

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
    , mipLevels(mipLevels)
    , layerCount(layerCount)
    , format(format)
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
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
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
    imageView = createImageView(*mRenderDevice, image, viewType, format, imageAspect, 0, mipLevels, 0, layerCount);
}

VulkanImage::~VulkanImage()
{
    if (mRenderDevice)
    {
        vkDestroyImageView(mRenderDevice->device, imageView, nullptr);
        for (auto iv : layerImageViews)
            vkDestroyImageView(mRenderDevice->device, iv, nullptr);
        for (auto iv : mipLevelImageViews)
            vkDestroyImageView(mRenderDevice->device, iv, nullptr);

        vkDestroyImage(mRenderDevice->device, image, nullptr);
        vkFreeMemory(mRenderDevice->device, memory, nullptr);

        width = 0;
        height = 0;
        mipLevels = 0;
        layerCount = 0;
        format = VK_FORMAT_UNDEFINED;

        mRenderDevice = nullptr;
    }
}

VulkanImage::VulkanImage(VulkanImage &&other) noexcept
    : VulkanImage()
{
    swap(other);
}

VulkanImage &VulkanImage::operator=(VulkanImage &&other) noexcept
{
    if (this != &other)
        swap(other);
    return *this;
}

void VulkanImage::swap(VulkanImage &other) noexcept
{
    std::swap(mRenderDevice, other.mRenderDevice);
    std::swap(image, other.image);
    std::swap(imageView, other.imageView);
    std::swap(memory, other.memory);
    std::swap(width, other.width);
    std::swap(height, other.height);
    std::swap(mipLevels, other.mipLevels);
    std::swap(layerCount, other.layerCount);
    std::swap(format, other.format);
    std::swap(imageAspect, other.imageAspect);
    std::swap(mipLevelImageViews, other.mipLevelImageViews);
}

void VulkanImage::createLayerImageViews(VkImageViewType viewType)
{
    assert(layerCount > 1);
    assert(layerImageViews.size() == 0);

    layerImageViews.resize(layerCount);
    for (uint32_t i = 0; i < layerCount; ++i)
    {
        layerImageViews.at(i) = createImageView(*mRenderDevice,
                                                image,
                                                viewType,
                                                format,
                                                imageAspect,
                                                0, mipLevels,
                                                i, 1);
    }
}

void VulkanImage::createMipLevelImageViews(VkImageViewType viewType)
{
    assert(mipLevels > 1);
    assert(mipLevelImageViews.size() == 0);

    mipLevelImageViews.resize(mipLevels);
    for (uint32_t i = 0; i < mipLevels; ++i)
    {
        mipLevelImageViews.at(i) = createImageView(*mRenderDevice,
                                                image,
                                                viewType,
                                                format,
                                                imageAspect,
                                                i, 1,
                                                0, layerCount);
    }
}

void VulkanImage::transitionLayout(VkCommandBuffer commandBuffer,
                                   VkImageLayout oldLayout, VkImageLayout newLayout,
                                   VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
                                   VkAccessFlags srcAccess, VkAccessFlags dstAccess)
{
    VkImageMemoryBarrier imageMemoryBarrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .srcAccessMask = srcAccess,
        .dstAccessMask = dstAccess,
        .oldLayout = oldLayout,
        .newLayout = newLayout,
        .image = image,
        .subresourceRange {
            .aspectMask = imageAspect,
            .baseMipLevel = 0,
            .levelCount = mipLevels, // transition all mip levels
            .baseArrayLayer = 0,
            .layerCount = layerCount // transition all layers
        }
    };

    vkCmdPipelineBarrier(commandBuffer,
                         srcStage, dstStage,
                         0, 0, nullptr, 0, nullptr,
                         1, &imageMemoryBarrier);
}

void VulkanImage::transitionLayout(VkImageLayout oldLayout, VkImageLayout newLayout,
                                   VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
                                   VkAccessFlags srcAccess, VkAccessFlags dstAccess)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(*mRenderDevice);
    transitionLayout(commandBuffer, oldLayout, newLayout, srcStage, dstStage, srcAccess, dstAccess);
    endSingleTimeCommands(*mRenderDevice, commandBuffer);
}

void VulkanImage::setDebugName(const std::string &debugName)
{
    setVulkanObjectDebugName(*mRenderDevice, VK_OBJECT_TYPE_IMAGE, debugName, image);
    setVulkanObjectDebugName(*mRenderDevice, VK_OBJECT_TYPE_IMAGE_VIEW, debugName, imageView);
}

void VulkanImage::copyBuffer(VkCommandBuffer commandBuffer, const VulkanBuffer &buffer, uint32_t layerIndex)
{
    VkBufferImageCopy copyRegion {
        .bufferOffset = 0,
        .bufferRowLength = width,
        .bufferImageHeight = height,
        .imageSubresource {
            .aspectMask = imageAspect,
            .mipLevel = 0,
            .baseArrayLayer = layerIndex,
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
                           image,
                           VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                           1, &copyRegion);
}

void VulkanImage::uploadImageData(VkCommandBuffer commandBuffer, const VulkanBuffer &buffer, uint32_t layerIndex)
{
    copyBuffer(commandBuffer, buffer, layerIndex);
}

void VulkanImage::uploadImageData(const void *data, uint32_t layerIndex)
{
    VkCommandBuffer commandBuffer = beginSingleTimeCommands(*mRenderDevice);

    VkDeviceSize size = imageMemoryDeviceSize(width, height, format);

    VulkanBuffer stagingBuffer(*mRenderDevice, size, BufferType::Staging, MemoryType::Host, data);

    copyBuffer(commandBuffer, stagingBuffer, layerIndex);

    endSingleTimeCommands(*mRenderDevice, commandBuffer);
}

VkImageView createImageView(const VulkanRenderDevice& renderDevice,
                            VkImage image,
                            VkImageViewType imageViewType,
                            VkFormat format,
                            VkImageAspectFlags aspectFlags,
                            uint32_t baseMipLevel,
                            uint32_t mipLevels,
                            uint32_t layerIndex,
                            uint32_t layerCount)
{
    VkImageView imageView;

    VkImageViewCreateInfo imageViewCreateInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .image = image,
        .viewType = imageViewType,
        .format = format,
        .components = {},
        .subresourceRange = {
            .aspectMask = aspectFlags,
            .baseMipLevel = baseMipLevel,
            .levelCount = mipLevels,
            .baseArrayLayer = layerIndex,
            .layerCount = layerCount
        }
    };

    VkResult result = vkCreateImageView(renderDevice.device, &imageViewCreateInfo, nullptr, &imageView);
    vulkanCheck(result, "Failed to create image view.");

    return imageView;
}
