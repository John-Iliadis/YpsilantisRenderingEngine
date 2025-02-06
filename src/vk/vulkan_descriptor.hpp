//
// Created by Gianni on 5/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_DESCRIPTOR_HPP
#define VULKANRENDERINGENGINE_VULKAN_DESCRIPTOR_HPP

#include "vulkan_render_device.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_texture.hpp"



class DescriptorSetLayoutBuilder
{
public:
    DescriptorSetLayoutBuilder();
    DescriptorSetLayoutBuilder(const VulkanRenderDevice& renderDevice);

    DescriptorSetLayoutBuilder& addLayoutBinding(uint32_t binding, VkDescriptorType type, VkShaderStageFlags stages);
    DescriptorSetLayoutBuilder& setDebugName(const std::string& name);
    VkDescriptorSetLayout create();

private:
    const VulkanRenderDevice* mRenderDevice;
    std::string mDebugName;
    std::vector<VkDescriptorSetLayoutBinding> mLayoutBindings;
};

class DescriptorSetBuilder
{
public:
    DescriptorSetBuilder();
    DescriptorSetBuilder(const VulkanRenderDevice& renderDevice);

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

    const VulkanRenderDevice* mRenderDevice;
    VkDescriptorSetLayout mDescriptorSetLayout;

    std::string mDebugName;
    std::vector<BufferInfo> mBufferInfos;
    std::vector<TextureInfo> mTextureInfos;
};

#endif //VULKANRENDERINGENGINE_VULKAN_DESCRIPTOR_HPP
