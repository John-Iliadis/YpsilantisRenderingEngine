//
// Created by Gianni on 5/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_DESCRIPTOR_HPP
#define VULKANRENDERINGENGINE_VULKAN_DESCRIPTOR_HPP

#include <vulkan/vulkan.h>
#include "vulkan_buffer.hpp"
#include "vulkan_texture.hpp"

struct DescriptorInfo
{
    VkDescriptorType descriptorType;
    VkShaderStageFlags shaderStageFlags;
};

struct BufferAttachment
{
    DescriptorInfo descriptorInfo;
    VulkanBuffer buffer;
    uint32_t offset;
    uint32_t size;
};

struct TextureAttachment
{
    DescriptorInfo descriptorInfo;
    VulkanTexture texture;
};

struct TextureArrayAttachment
{
    DescriptorInfo descriptorInfo;
    std::vector<VulkanTexture> textureArray;
};

struct DescriptorSetInfo
{
    std::vector<BufferAttachment> buffers;
    std::vector<TextureAttachment> textures;
    std::vector<TextureArrayAttachment> textureArray;
};

BufferAttachment createBufferAttachment(VkDescriptorType type,
                                        VkShaderStageFlags shaderStageFlags,
                                        const VulkanBuffer& buffer,
                                        uint32_t offset, uint32_t size);

TextureAttachment createTextureAttachment(VkDescriptorType type,
                                          VkShaderStageFlags shaderStageFlags,
                                          const VulkanTexture& texture);

TextureArrayAttachment createTextureArrayAttachment(VkDescriptorType type,
                                                    VkShaderStageFlags shaderStageFlags,
                                                    const std::vector<VulkanTexture>& textureArray);

VkDescriptorPool createDescriptorPool(const VulkanRenderDevice& renderDevice,
                                      uint32_t imageSamplerCount,
                                      uint32_t uniformBufferCount,
                                      uint32_t storageBufferCount,
                                      uint32_t inputAttachmentCount,
                                      uint32_t maxSets);

VkDescriptorPoolSize descriptorPoolSize(VkDescriptorType type, uint32_t count);

#endif //VULKANRENDERINGENGINE_VULKAN_DESCRIPTOR_HPP
