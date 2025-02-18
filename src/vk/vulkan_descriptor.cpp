//
// Created by Gianni on 5/01/2025.
//

#include "vulkan_descriptor.hpp"

void setDsLayoutDebugName(const VulkanRenderDevice& renderDevice, VkDescriptorSetLayout dsLayout, const std::string& debugName)
{
    setVulkanObjectDebugName(renderDevice, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT, debugName, dsLayout);
}

void setDSDebugName(const VulkanRenderDevice& renderDevice, VkDescriptorSet ds, const std::string& debugName)
{
    setVulkanObjectDebugName(renderDevice, VK_OBJECT_TYPE_DESCRIPTOR_SET, debugName, ds);
}
