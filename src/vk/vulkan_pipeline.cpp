//
// Created by Gianni on 5/01/2025.
//

#include "vulkan_pipeline.hpp"

VkPipelineShaderStageCreateInfo shaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule module)
{
    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = stage,
        .module = module,
        .pName = "main"
    };
}
