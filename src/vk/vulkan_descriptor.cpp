//
// Created by Gianni on 5/01/2025.
//

#include "vulkan_descriptor.hpp"

BufferAttachment createBufferAttachment(VkDescriptorType type,
                                        VkShaderStageFlags shaderStageFlags,
                                        VkBuffer buffer,
                                        uint32_t offset, uint32_t size)
{
    return {
        .descriptorInfo {
            .descriptorType = type,
            .shaderStageFlags = shaderStageFlags
        },
        .buffer = buffer,
        .offset = offset,
        .range = size
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

void createDescriptorSet(const VulkanRenderDevice& renderDevice,
                         const DescriptorSetInfo& descriptorSetInfo,
                         VkDescriptorPool descriptorPool,
                         VkDescriptorSetLayout& outDescriptorSetLayout,
                         VkDescriptorSet& outDescriptorSet)
{
    /* -- descriptor set layout -- */
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    std::vector<VkDescriptorBufferInfo> bufferInfos;
    std::vector<VkDescriptorImageInfo> imageInfos;
    std::vector<std::vector<VkDescriptorImageInfo>> imageArrayInfos

    // buffer infos
    for (const BufferAttachment& bufferAttachment : descriptorSetInfo.buffers)
    {
        VkDescriptorSetLayoutBinding layoutBinding {
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = bufferAttachment.descriptorInfo.descriptorType,
            .descriptorCount = 1,
            .stageFlags = bufferAttachment.descriptorInfo.shaderStageFlags
        };

        bindings.push_back(layoutBinding);

        VkDescriptorBufferInfo descriptorBufferInfo {
            .buffer = bufferAttachment.buffer,
            .offset = bufferAttachment.offset,
            .range = bufferAttachment.range
        };

        bufferInfos.push_back(descriptorBufferInfo);
    }

    // texture infos
    for (const TextureAttachment& textureAttachment : descriptorSetInfo.textures)
    {
        VkDescriptorSetLayoutBinding layoutBinding {
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = textureAttachment.descriptorInfo.descriptorType,
            .descriptorCount = 1,
            .stageFlags = textureAttachment.descriptorInfo.shaderStageFlags
        };

        bindings.push_back(layoutBinding);

        VkDescriptorImageInfo descriptorImageInfo {
            .sampler = textureAttachment.texture.sampler,
            .imageView = textureAttachment.texture.image.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        imageInfos.push_back(descriptorImageInfo);
    }

    // texture array infos
    for (const TextureArrayAttachment& textureArrayAttachment : descriptorSetInfo.textureArrays)
    {
        VkDescriptorSetLayoutBinding layoutBinding {
            .binding = static_cast<uint32_t>(bindings.size()),
            .descriptorType = textureArrayAttachment.descriptorInfo.descriptorType,
            .descriptorCount = static_cast<uint32_t>(textureArrayAttachment.textureArray.size()),
            .stageFlags = textureArrayAttachment.descriptorInfo.shaderStageFlags
        };

        bindings.push_back(layoutBinding);

        std::vector<VkDescriptorImageInfo> lImageInfos;
        for (const VulkanTexture& texture : textureArrayAttachment.textureArray)
        {
            VkDescriptorImageInfo descriptorImageInfo {
                .sampler = texture.sampler,
                .imageView = texture.image.imageView,
                .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            };

            lImageInfos.push_back(descriptorImageInfo);
        }

        imageArrayInfos.push_back(std::move(lImageInfos));
    }

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(bindings.size()),
        .pBindings = bindings.data()
    };

    VkResult result = vkCreateDescriptorSetLayout(renderDevice.device,
                                                  &descriptorSetLayoutCreateInfo,
                                                  nullptr,
                                                  &outDescriptorSetLayout);
    vulkanCheck(result, "Failed to create descriptor set layout.");

    /* -- descriptor set -- */
    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &outDescriptorSetLayout
    };

    result = vkAllocateDescriptorSets(renderDevice.device, &descriptorSetAllocateInfo, &outDescriptorSet);
    vulkanCheck(result, "Failed to allocate descriptor set.");

    // descriptor writes
    std::vector<VkWriteDescriptorSet> descriptorWrites;

    for (uint32_t i = 0; i < bufferInfos.size(); ++i)
    {
        VkWriteDescriptorSet writeDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = outDescriptorSet,
            .dstBinding = i,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = descriptorSetInfo.buffers.at(i).descriptorInfo.descriptorType,
            .pBufferInfo = &bufferInfos.at(i)
        };

        descriptorWrites.push_back(writeDescriptorSet);
    }

    for (uint32_t i = 0, descriptorIndex = bufferInfos.size(); i < imageInfos.size(); ++i, ++descriptorIndex)
    {
        VkWriteDescriptorSet writeDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = outDescriptorSet,
            .dstBinding = descriptorIndex,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = descriptorSetInfo.textures.at(i).descriptorInfo.descriptorType,
            .pImageInfo = &imageInfos.at(i)
        };

        descriptorWrites.push_back(writeDescriptorSet);
    }

    for (uint32_t i = 0, descriptorIndex = bufferInfos.size() + imageInfos.size(); i < imageArrayInfos.size(); ++i, ++descriptorIndex)
    {
        VkWriteDescriptorSet writeDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = outDescriptorSet,
            .dstBinding = descriptorIndex,
            .dstArrayElement = 0,
            .descriptorCount = static_cast<uint32_t>(imageArrayInfos.at(i).size()),
            .descriptorType = descriptorSetInfo.textureArrays.at(i).descriptorInfo.descriptorType,
            .pImageInfo = imageArrayInfos.at(i).data()
        };

        descriptorWrites.push_back(writeDescriptorSet);
    }

    vkUpdateDescriptorSets(renderDevice.device, descriptorWrites.size(), descriptorWrites.data(), 0, 0);
}

VkDescriptorPoolSize descriptorPoolSize(VkDescriptorType type, uint32_t count)
{
    return {
        .type = type,
        .descriptorCount = count
    };
}
