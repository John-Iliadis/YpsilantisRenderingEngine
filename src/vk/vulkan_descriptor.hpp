//
// Created by Gianni on 5/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_DESCRIPTOR_HPP
#define VULKANRENDERINGENGINE_VULKAN_DESCRIPTOR_HPP

#include "vulkan_render_device.hpp"
#include "vulkan_buffer.hpp"
#include "vulkan_texture.hpp"

struct DsLayoutSpecification
{
    VkDescriptorSetLayoutCreateFlagBits flags;
    const void* pNext;
    std::vector<VkDescriptorSetLayoutBinding> bindings;
    std::optional<std::string> debugName;
};

class VulkanDsLayout
{
public:
    VulkanDsLayout();
    VulkanDsLayout(const VulkanRenderDevice& renderDevice, const DsLayoutSpecification& specification);
    ~VulkanDsLayout();

    VulkanDsLayout(const VulkanDsLayout&) = delete;
    VulkanDsLayout& operator=(const VulkanDsLayout&) = delete;

    VulkanDsLayout(VulkanDsLayout&& other) noexcept;
    VulkanDsLayout& operator=(VulkanDsLayout&& other) noexcept;

    void swap(VulkanDsLayout& other);

    operator VkDescriptorSetLayout() const;

private:
    const VulkanRenderDevice* mRenderDevice;
    VkDescriptorSetLayout mDsLayout;
};

class VulkanDs
{
public:

private:
};

#endif //VULKANRENDERINGENGINE_VULKAN_DESCRIPTOR_HPP
