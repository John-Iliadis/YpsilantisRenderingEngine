//
// Created by Gianni on 3/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_TEXTURE_HPP
#define VULKANRENDERINGENGINE_VULKAN_TEXTURE_HPP

#include <vulkan/vulkan.h>
#include "vulkan_image.hpp"

struct VulkanTexture
{
    VulkanImage image;
    VkSampler sampler;
    uint32_t width;
    uint32_t height;
};


#endif //VULKANRENDERINGENGINE_VULKAN_TEXTURE_HPP
