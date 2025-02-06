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

// -- DescriptorSetLayoutBuilder -- //

DescriptorSetLayoutBuilder::DescriptorSetLayoutBuilder()
    : mRenderDevice()
{
}

DescriptorSetLayoutBuilder::DescriptorSetLayoutBuilder(const VulkanRenderDevice &renderDevice)
    : mRenderDevice(&renderDevice)
{
}

DescriptorSetLayoutBuilder& DescriptorSetLayoutBuilder::addLayoutBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stages)
{
    mLayoutBindings.emplace_back(binding, type, 1, stages);
    return *this;
}

DescriptorSetLayoutBuilder &DescriptorSetLayoutBuilder::setDebugName(const std::string &name)
{
    mDebugName = name;
    return *this;
}

VkDescriptorSetLayout DescriptorSetLayoutBuilder::create()
{
    VkDescriptorSetLayout descriptorSetLayout;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = static_cast<uint32_t>(mLayoutBindings.size()),
        .pBindings = mLayoutBindings.data()
    };

    VkResult result = vkCreateDescriptorSetLayout(mRenderDevice->device,
                                                  &descriptorSetLayoutCreateInfo,
                                                  nullptr,
                                                  &descriptorSetLayout);
    vulkanCheck(result, "Failed to create descriptor set layout.");

    if (!mDebugName.empty())
    {
        setVulkanObjectDebugName(*mRenderDevice,
                                 VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                                 mDebugName,
                                 descriptorSetLayout);
    }

    mLayoutBindings.clear();

    return descriptorSetLayout;
}

// -- DescriptorSetBuilder -- //

DescriptorSetBuilder::DescriptorSetBuilder()
    : mRenderDevice()
    , mDescriptorSetLayout()
{
}

DescriptorSetBuilder::DescriptorSetBuilder(const VulkanRenderDevice &renderDevice)
    : mRenderDevice(&renderDevice)
    , mDescriptorSetLayout()
{
}

DescriptorSetBuilder& DescriptorSetBuilder::setLayout(VkDescriptorSetLayout layout)
{
    mDescriptorSetLayout = layout;
    return *this;
}

DescriptorSetBuilder& DescriptorSetBuilder::addBuffer(VkDescriptorType type, uint32_t binding, VkBuffer buffer, uint32_t offset, uint32_t range)
{
    mBufferInfos.emplace_back(type, binding, buffer, offset, range);
    return *this;
}

DescriptorSetBuilder& DescriptorSetBuilder::addTexture(VkDescriptorType type, uint32_t binding, const VulkanTexture &texture)
{
    mTextureInfos.emplace_back(type, binding, texture);
    return *this;
}

DescriptorSetBuilder &DescriptorSetBuilder::setDebugName(const std::string &name)
{
    mDebugName = name;
    return *this;
}

VkDescriptorSet DescriptorSetBuilder::create()
{
    VkDescriptorSet descriptorSet;

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = mRenderDevice->descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &mDescriptorSetLayout
    };

    VkResult result = vkAllocateDescriptorSets(mRenderDevice->device, &descriptorSetAllocateInfo, &descriptorSet);
    vulkanCheck(result, "Failed to allocate descriptor set.");

    // descriptor writes
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
            .sampler = mTextureInfos.at(i).texture.vulkanSampler.sampler,
            .imageView = mTextureInfos.at(i).texture.vulkanImage.imageView,
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

    vkUpdateDescriptorSets(mRenderDevice->device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);

    if (!mDebugName.empty())
    {
        setVulkanObjectDebugName(*mRenderDevice,
                                 VK_OBJECT_TYPE_DESCRIPTOR_SET,
                                 mDebugName,
                                 descriptorSet);
    }

    clear();

    return descriptorSet;
}

void DescriptorSetBuilder::clear()
{
    mBufferInfos.clear();
    mTextureInfos.clear();
}
