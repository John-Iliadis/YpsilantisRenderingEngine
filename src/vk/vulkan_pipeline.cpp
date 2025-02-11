//
// Created by Gianni on 5/01/2025.
//

#include "vulkan_pipeline.hpp"

VulkanShaderModule::VulkanShaderModule(const VulkanRenderDevice& renderDevice, const std::string& path)
    : mRenderDevice(renderDevice)
    , mShaderModule()
{
    std::ifstream shaderFile(path, std::ios::binary | std::ios::ate);

    check(shaderFile.is_open(), std::format("Failed to open {}", path).c_str());

    std::streamsize bufferSize = shaderFile.tellg();
    std::vector<uint32_t> shaderSrc(bufferSize / 4);

    shaderFile.seekg(0);
    shaderFile.read(reinterpret_cast<char*>(shaderSrc.data()), bufferSize);

    VkShaderModuleCreateInfo shaderModuleCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = 4 * shaderSrc.size(),
        .pCode = shaderSrc.data()
    };

    VkResult result = vkCreateShaderModule(mRenderDevice.device, &shaderModuleCreateInfo, nullptr, &mShaderModule);
    vulkanCheck(result, "Failed to create shader module.");

    setVulkanObjectDebugName(mRenderDevice,
                             VK_OBJECT_TYPE_SHADER_MODULE,
                             path,
                             mShaderModule);
}

VulkanShaderModule::~VulkanShaderModule()
{
    vkDestroyShaderModule(mRenderDevice.device, mShaderModule, nullptr);
}

VulkanShaderModule::operator VkShaderModule() const
{
    return mShaderModule;
}


VkPipelineShaderStageCreateInfo shaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule module)
{
    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = stage,
        .module = module,
        .pName = "main"
    };
}

void setPipelineDebugName(const VulkanRenderDevice& renderDevice, VkPipeline pipeline, const std::string& name)
{
    setVulkanObjectDebugName(renderDevice, VK_OBJECT_TYPE_PIPELINE, name, pipeline);
}

void setPipelineLayoutDebugName(const VulkanRenderDevice& renderDevice, VkPipelineLayout layout, const std::string& name)
{
    setVulkanObjectDebugName(renderDevice, VK_OBJECT_TYPE_PIPELINE_LAYOUT, name, layout);
}