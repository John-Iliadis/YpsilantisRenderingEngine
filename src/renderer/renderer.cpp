//
// Created by Gianni on 4/01/2025.
//

#include "renderer.hpp"

Renderer::Renderer(const VulkanRenderDevice &renderDevice)
    : mRenderDevice(renderDevice)
    , mRenderpass()
    , mPipelineLayout()
    , mGraphicsPipeline()
    , mDescriptorPool()
    , mDescriptorSetLayout()
    , mDescriptorSet()
{
}

Renderer::~Renderer()
{
    for (VkFramebuffer fb : mFramebuffers)
        vkDestroyFramebuffer(mRenderDevice.device, fb, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mRenderpass, nullptr);
    vkDestroyPipelineLayout(mRenderDevice.device, mPipelineLayout, nullptr);
    vkDestroyPipeline(mRenderDevice.device, mGraphicsPipeline, nullptr);
    vkDestroyDescriptorSetLayout(mRenderDevice.device, mDescriptorSetLayout, nullptr);
    vkDestroyDescriptorPool(mRenderDevice.device, mDescriptorPool, nullptr);
}
