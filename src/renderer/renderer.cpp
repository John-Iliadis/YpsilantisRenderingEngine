//
// Created by Gianni on 4/01/2025.
//

#include "renderer.hpp"

Renderer::Renderer()
    : mRenderDevice()
    , mRenderpass()
    , mPipelineLayout()
    , mGraphicsPipeline()
{
}

void Renderer::init(const VulkanRenderDevice &renderDevice)
{
    mRenderDevice = renderDevice;
}

void Renderer::terminate()
{
    for (VkFramebuffer fb : mFramebuffers)
        vkDestroyFramebuffer(mRenderDevice.device, fb, nullptr);
    vkDestroyRenderPass(mRenderDevice.device, mRenderpass, nullptr);
    vkDestroyPipelineLayout(mRenderDevice.device, mPipelineLayout, nullptr);
    vkDestroyPipeline(mRenderDevice.device, mGraphicsPipeline, nullptr);
}
