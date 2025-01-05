//
// Created by Gianni on 4/01/2025.
//

#ifndef VULKANRENDERINGENGINE_RENDERER_HPP
#define VULKANRENDERINGENGINE_RENDERER_HPP

#include <vulkan/vulkan.h>
#include "../vk/include.hpp"

class Renderer
{
public:
    Renderer(const VulkanRenderDevice& renderDevice);
    virtual ~Renderer();

    virtual void fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) = 0;

protected:
    virtual void beginRenderpass(VkCommandBuffer commandBuffer, uint32_t imageIndex) = 0;

protected:
    VulkanRenderDevice mRenderDevice;

    VkRenderPass mRenderpass;
    std::vector<VkFramebuffer> mFramebuffers;

    VkPipelineLayout mPipelineLayout;
    VkPipeline mGraphicsPipeline;

    VkDescriptorPool mDescriptorPool;
    VkDescriptorSetLayout mDescriptorSetLayout;
    VkDescriptorSet mDescriptorSet;
};


#endif //VULKANRENDERINGENGINE_RENDERER_HPP
