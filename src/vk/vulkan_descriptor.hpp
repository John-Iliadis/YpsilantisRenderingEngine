//
// Created by Gianni on 5/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_DESCRIPTOR_HPP
#define VULKANRENDERINGENGINE_VULKAN_DESCRIPTOR_HPP

#include <vulkan/vulkan.h>
#include "vulkan_buffer.hpp"
#include "vulkan_texture.hpp"

VkDescriptorPool createDescriptorPool(const VulkanRenderDevice& renderDevice,
                                      uint32_t imageSamplerCount,
                                      uint32_t uniformBufferCount,
                                      uint32_t storageBufferCount,
                                      uint32_t maxSets);

VkDescriptorPoolSize descriptorPoolSize(VkDescriptorType type, uint32_t count);

class DescriptorSetLayoutCreator
{
public:
    DescriptorSetLayoutCreator(VkDevice device);

    void addLayoutBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stages);

    VkDescriptorSetLayout create() const;

private:
    VkDevice mDevice;
    std::vector<VkDescriptorSetLayoutBinding> mLayoutBindings;
};

class DescriptorSetCreator
{
public:
    DescriptorSetCreator(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout layout);

    void addBuffer(VkDescriptorType type, uint32_t binding, VkBuffer buffer, uint32_t offset, uint32_t range);
    void addTexture(VkDescriptorType type, uint32_t binding, const VulkanTexture& texture);

    VkDescriptorSet create() const;

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
    VkDevice mDevice;
    VkDescriptorPool mDescriptorPool;
    VkDescriptorSetLayout mDescriptorSetLayout;
    std::vector<BufferInfo> mBufferInfos;
    std::vector<TextureInfo> mTextureInfos;
};

#endif //VULKANRENDERINGENGINE_VULKAN_DESCRIPTOR_HPP
