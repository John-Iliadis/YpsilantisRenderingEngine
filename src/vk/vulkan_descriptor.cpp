//
// Created by Gianni on 5/01/2025.
//

#include "vulkan_descriptor.hpp"

BufferAttachment createBufferAttachment(VkDescriptorType type,
                                        VkShaderStageFlags shaderStageFlags,
                                        const VulkanBuffer& buffer,
                                        uint32_t offset, uint32_t size)
{
    return {
        .descriptorInfo {
            .descriptorType = type,
            .shaderStageFlags = shaderStageFlags
        },
        .buffer = buffer,
        .offset = offset,
        .size = size
    };
}

TextureAttachment createTextureAttachment(VkDescriptorType type,
                                          VkShaderStageFlags shaderStageFlags,
                                          const VulkanTexture& texture)
{
    return {
        .descriptorInfo {
            .descriptorType = type,
            .shaderStageFlags = shaderStageFlags
        },
        .texture = texture
    };
}

TextureArrayAttachment createTextureArrayAttachment(VkDescriptorType type,
                                                    VkShaderStageFlags shaderStageFlags,
                                                    const std::vector<VulkanTexture>& textureArray)
{
    return {
        .descriptorInfo {
            .descriptorType = type,
            .shaderStageFlags = shaderStageFlags
        },
        .textureArray = textureArray
    };
}

VkDescriptorPool createDescriptorPool(const VulkanRenderDevice& renderDevice,
                                      uint32_t imageSamplerCount,
                                      uint32_t uniformBufferCount,
                                      uint32_t storageBufferCount,
                                      uint32_t inputAttachmentCount,
                                      uint32_t maxSets)
{
    VkDescriptorPool descriptorPool;

    std::array<VkDescriptorPoolSize, 4> descriptorPoolSizes {{
        descriptorPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, imageSamplerCount),
        descriptorPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uniformBufferCount),
        descriptorPoolSize(VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, storageBufferCount),
        descriptorPoolSize(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, inputAttachmentCount)
    }};

    VkDescriptorPoolCreateInfo descriptorPoolCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
        .maxSets = 1,
        .poolSizeCount = descriptorPoolSizes.size(),
        .pPoolSizes = descriptorPoolSizes.data()
    };

    VkResult result = vkCreateDescriptorPool(renderDevice.device, &descriptorPoolCreateInfo, nullptr, &descriptorPool);
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
