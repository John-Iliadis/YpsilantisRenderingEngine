//
// Created by Gianni on 5/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_DESCRIPTOR_HPP
#define VULKANRENDERINGENGINE_VULKAN_DESCRIPTOR_HPP

#include <vulkan/vulkan.h>
#include "vulkan_buffer.hpp"
#include "vulkan_texture.hpp"

VkDescriptorPool createDescriptorPool(VkDevice device,
                                      uint32_t imageSamplerCount,
                                      uint32_t uniformBufferCount,
                                      uint32_t storageBufferCount,
                                      uint32_t maxSets);

VkDescriptorPoolSize descriptorPoolSize(VkDescriptorType type, uint32_t count);

class DescriptorSetLayoutBuilder
{
public:
    DescriptorSetLayoutBuilder();

    void init(VkDevice device);

    DescriptorSetLayoutBuilder& addLayoutBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stages);
    DescriptorSetLayoutBuilder& setDebugName(const std::string& name);
    VkDescriptorSetLayout create();

private:
    VkDevice mDevice;
    std::string mDebugName;
    std::vector<VkDescriptorSetLayoutBinding> mLayoutBindings;
};

class DescriptorSetBuilder
{
public:
    DescriptorSetBuilder();

    void init(VkDevice device, VkDescriptorPool descriptorPool);

    DescriptorSetBuilder& setLayout(VkDescriptorSetLayout layout);
    DescriptorSetBuilder& addBuffer(VkDescriptorType type, uint32_t binding, VkBuffer buffer, uint32_t offset, uint32_t range);
    DescriptorSetBuilder& addTexture(VkDescriptorType type, uint32_t binding, const VulkanTexture& texture);
    DescriptorSetBuilder& setDebugName(const std::string& name);
    VkDescriptorSet create();

private:
    struct BufferInfo
    {
        VkDescriptorType type;
        uint32_t binding;
        VkBuffer buffer;
        uint32_t offset;
        uint32_t range;
    };

    struct TextureInfo
    {
        VkDescriptorType type;
        uint32_t binding;
        const VulkanTexture& texture;
    };

private:
    void clear();

    VkDevice mDevice;
    VkDescriptorPool mDescriptorPool;
    VkDescriptorSetLayout mDescriptorSetLayout;

    std::string mDebugName;
    std::vector<BufferInfo> mBufferInfos;
    std::vector<TextureInfo> mTextureInfos;
};

#endif //VULKANRENDERINGENGINE_VULKAN_DESCRIPTOR_HPP
