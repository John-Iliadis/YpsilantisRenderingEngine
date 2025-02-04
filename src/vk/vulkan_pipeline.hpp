//
// Created by Gianni on 5/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_PIPELINE_HPP
#define VULKANRENDERINGENGINE_VULKAN_PIPELINE_HPP

#include "vulkan_render_device.hpp"
#include "vulkan_utils.hpp"
#include "../utils/utils.hpp"

class VulkanShaderModule
{
public:
    VulkanShaderModule(const VulkanRenderDevice& renderDevice, const std::string& path);
    ~VulkanShaderModule();

    operator VkShaderModule() const;

private:
    const VulkanRenderDevice& mRenderDevice;
    VkShaderModule mShaderModule;
};

VkPipelineShaderStageCreateInfo shaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule module);

#endif //VULKANRENDERINGENGINE_VULKAN_PIPELINE_HPP
