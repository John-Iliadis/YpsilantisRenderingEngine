//
// Created by Gianni on 5/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_DESCRIPTOR_HPP
#define VULKANRENDERINGENGINE_VULKAN_DESCRIPTOR_HPP

#include "vulkan_render_device.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_texture.hpp"

VkDescriptorPool createDescriptorPool(VkDevice device,
                                      uint32_t imageSamplerCount,
                                      uint32_t uniformBufferCount,
                                      uint32_t storageBufferCount,
                                      uint32_t maxSets);

VkDescriptorPoolSize descriptorPoolSize(VkDescriptorType type, uint32_t count);

VkDescriptorSetLayoutBinding dsLayoutBinding(uint32_t binding,
                                             VkDescriptorType type,
                                             uint32_t descriptorCount,
                                             VkShaderStageFlags shaderStages);

void setDsLayoutDebugName(const VulkanRenderDevice& renderDevice,
                          VkDescriptorSetLayout dsLayout,
                          const std::string& debugName);

void setDSDebugName(const VulkanRenderDevice& renderDevice, VkDescriptorSet ds, const std::string& debugName);

VkDescriptorBufferInfo descriptorBufferInfo(VkBuffer buffer, uint32_t offset, uint32_t range);

VkDescriptorImageInfo descriptorImageInfo(VkSampler sampler, VkImageView imageView, VkImageLayout imageLayout);

#endif //VULKANRENDERINGENGINE_VULKAN_DESCRIPTOR_HPP
