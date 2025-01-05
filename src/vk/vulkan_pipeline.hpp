//
// Created by Gianni on 5/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_PIPELINE_HPP
#define VULKANRENDERINGENGINE_VULKAN_PIPELINE_HPP

#include <vulkan/vulkan.h>

VkPipelineShaderStageCreateInfo shaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule module);

#endif //VULKANRENDERINGENGINE_VULKAN_PIPELINE_HPP
