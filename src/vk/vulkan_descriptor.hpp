//
// Created by Gianni on 5/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_DESCRIPTOR_HPP
#define VULKANRENDERINGENGINE_VULKAN_DESCRIPTOR_HPP

#include "vulkan_render_device.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_texture.hpp"

void setDsLayoutDebugName(const VulkanRenderDevice& renderDevice,
                          VkDescriptorSetLayout dsLayout,
                          const std::string& debugName);

void setDSDebugName(const VulkanRenderDevice& renderDevice, VkDescriptorSet ds, const std::string& debugName);

#endif //VULKANRENDERINGENGINE_VULKAN_DESCRIPTOR_HPP
