//
// Created by Gianni on 5/01/2025.
//

#include "vulkan_descriptor.hpp"

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

DescriptorSetLayoutCreator::DescriptorSetLayoutCreator(VkDevice device)
    : mDevice(device)
{
}

void DescriptorSetLayoutCreator::addLayoutBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stages)
{
    mLayoutBindings.emplace_back(binding, type, 1, stages);
}

VkDescriptorSetLayout DescriptorSetLayoutCreator::create() const
{
    VkDescriptorSetLayout descriptorSetLayout;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(mLayoutBindings.size()),
        .pBindings = mLayoutBindings.data()
    };

    VkResult result = vkCreateDescriptorSetLayout(mDevice,
                                                  &descriptorSetLayoutCreateInfo,
                                                  nullptr,
                                                  &descriptorSetLayout);
    vulkanCheck(result, "Failed to create descriptor set layout.");

    return descriptorSetLayout;
}

DescriptorSetCreator::DescriptorSetCreator(VkDevice device,
                                           VkDescriptorPool descriptorPool,
                                           VkDescriptorSetLayout layout)
    : mDevice(device)
    , mDescriptorPool(descriptorPool)
    , mDescriptorSetLayout(layout)
{
}

void DescriptorSetCreator::addBuffer(VkDescriptorType type, uint32_t binding, VkBuffer buffer, uint32_t offset, uint32_t range)
{
    mBufferInfos.emplace_back(type, binding, buffer, offset, range);
}

void DescriptorSetCreator::addTexture(VkDescriptorType type, uint32_t binding, const VulkanTexture &texture)
{
    mTextureInfos.emplace_back(type, binding, texture);
}

VkDescriptorSet DescriptorSetCreator::create() const
{
    VkDescriptorSet descriptorSet;

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = mDescriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &mDescriptorSetLayout
    };

    VkResult result = vkAllocateDescriptorSets(mDevice, &descriptorSetAllocateInfo, &descriptorSet);
    vulkanCheck(result, "Failed to allocate descriptor set.");

    // writes
    std::vector<VkWriteDescriptorSet> descriptorWrites;

    for (uint32_t i = 0; i < mBufferInfos.size(); ++i)
    {
        VkDescriptorBufferInfo bufferInfo {
            .buffer = mBufferInfos.at(i).buffer,
            .offset = mBufferInfos.at(i).offset,
            .range = mBufferInfos.at(i).range
        };

        VkWriteDescriptorSet writeDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = mBufferInfos.at(i).binding,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = mBufferInfos.at(i).type,
            .pBufferInfo = &bufferInfo
        };

        descriptorWrites.push_back(writeDescriptorSet);
    }

    for (uint32_t i = 0; i < mTextureInfos.size(); ++i)
    {
        VkDescriptorImageInfo imageInfo {
            .sampler = mTextureInfos.at(i).texture.sampler,
            .imageView = mTextureInfos.at(i).texture.image.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        VkWriteDescriptorSet writeDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = descriptorSet,
            .dstBinding = mTextureInfos.at(i).binding,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = mTextureInfos.at(i).type,
            .pImageInfo = &imageInfo
        };

        descriptorWrites.push_back(writeDescriptorSet);
    }

    vkUpdateDescriptorSets(mDevice, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);

    return descriptorSet;
}
