//
// Created by Gianni on 3/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_IMAGE_HPP
#define VULKANRENDERINGENGINE_VULKAN_IMAGE_HPP

#include <vulkan/vulkan.h>
#include "vulkan_device.hpp"

struct VulkanImage
{
    VkImage image;
    VkImageView imageView;
    VkDeviceMemory memory;
};

VkImageView createImageView(const VulkanDevice& device,
                            VkImage image,
                            VkImageViewType viewType,
                            VkFormat format,
                            VkImageAspectFlags aspectFlags,
                            uint32_t mipLevels = 1,
                            uint32_t layerCount = 1);

#endif //VULKANRENDERINGENGINE_VULKAN_IMAGE_HPP
