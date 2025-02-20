//
// Created by Gianni on 5/01/2025.
//

#include "vulkan_descriptor.hpp"

// -- VulkanDsLayout -- //

VulkanDsLayout::VulkanDsLayout()
    : mRenderDevice()
    , mDsLayout()
{
}

VulkanDsLayout::VulkanDsLayout(const VulkanRenderDevice &renderDevice, const DsLayoutSpecification &specification)
    : mRenderDevice(&renderDevice)
{
    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = specification.pNext,
        .flags = specification.flags,
        .bindingCount = static_cast<uint32_t>(specification.bindings.size()),
        .pBindings = specification.bindings.data()
    };

    VkResult result = vkCreateDescriptorSetLayout(renderDevice.device,
                                                  &descriptorSetLayoutCreateInfo,
                                                  nullptr,
                                                  &mDsLayout);
    vulkanCheck(result, "Failed to create ds layout");

    if (const auto& debugName = specification.debugName)
    {
        setVulkanObjectDebugName(renderDevice,
                                 VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                                 *debugName,
                                 mDsLayout);
    }
}

VulkanDsLayout::~VulkanDsLayout()
{
    if (mRenderDevice)
    {
        vkDestroyDescriptorSetLayout(mRenderDevice->device, mDsLayout, nullptr);
        mRenderDevice = nullptr;
    }
}

VulkanDsLayout::VulkanDsLayout(VulkanDsLayout &&other) noexcept
    : VulkanDsLayout()
{
    swap(other);
}

VulkanDsLayout &VulkanDsLayout::operator=(VulkanDsLayout &&other) noexcept
{
    if (this != &other)
        swap(other);
    return *this;
}

void VulkanDsLayout::swap(VulkanDsLayout &other)
{
    std::swap(mRenderDevice, other.mRenderDevice);
    std::swap(mDsLayout, other.mDsLayout);
}

VulkanDsLayout::operator VkDescriptorSetLayout() const
{
    return mDsLayout;
}

// -- VulkanDs -- //
