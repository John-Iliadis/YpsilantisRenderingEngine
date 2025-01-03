//
// Created by Gianni on 3/01/2025.
//

#include "vulkan_image.hpp"

VkImageView createImageView(const VulkanDevice& device,
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

    VkResult result = vkCreateImageView(device.device, &imageViewCreateInfo, nullptr, &imageView);
    vulkanCheck(result, "Failed to create image view.");

    return imageView;
}
