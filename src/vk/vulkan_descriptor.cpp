//
// Created by Gianni on 5/01/2025.
//

#include "vulkan_descriptor.hpp"

VkDescriptorPool createDescriptorPool(VkDevice device,
                                      uint32_t imageSamplerCount,
                                      uint32_t uniformBufferCount,
                                      uint32_t storageBufferCount,
                                      uint32_t maxSets)
{
    VkDescriptorPool descriptorPool;

    std::array<VkDescriptorPoolSize, 3> descriptorPoolSizes {{
        descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageSamplerCount),
        descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uniformBufferCount),
        descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, storageBufferCount),
    }};

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
        .maxSets = maxSets,
        .poolSizeCount = descriptorPoolSizes.size(),
        .pPoolSizes = descriptorPoolSizes.data()
    };

    VkResult result = vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, nullptr, &descriptorPool);
    vulkanCheck(result, "Failed to create descriptor pool.");

    return descriptorPool;
}

VkDescriptorPoolSize descriptorPoolSize(VkDescriptorType type, uint32_t count)
{
    return {
        .type = type,
        .descriptorCount = count
    };
}

VkDescriptorSetLayoutBinding dsLayoutBinding(uint32_t binding,
                                             VkDescriptorType type,
                                             uint32_t descriptorCount,
                                             VkShaderStageFlags shaderStages)
{
    return {
        .binding = binding,
        .descriptorType = type,
        .descriptorCount = descriptorCount,
        .stageFlags = shaderStages
    };
}

void setDSLayoutDebugName(const VulkanRenderDevice& renderDevice, VkDescriptorSetLayout dsLayout, const std::string& debugName)
{
    setVulkanObjectDebugName(renderDevice, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, debugName, dsLayout);
}

void setDSDebugName(const VulkanRenderDevice& renderDevice, VkDescriptorSet ds, const std::string& debugName)
{
    setVulkanObjectDebugName(renderDevice, VK_OBJECT_TYPE_DESCRIPTOR_SET, debugName, ds);
}

VkDescriptorBufferInfo descriptorBufferInfo(VkBuffer buffer, uint32_t offset, uint32_t range)
{
    return {
        .buffer = buffer,
        .offset = offset,
        .range = range
    };
}

VkDescriptorImageInfo descriptorImageInfo(VkSampler sampler, VkImageView imageView, VkImageLayout imageLayout)
{
    return {
        .sampler = sampler,
        .imageView = imageView,
        .imageLayout = imageLayout
    };
}
