//
// Created by Gianni on 2/01/2025.
//

#include "application.hpp"

static constexpr int sInitialWindowWidth = 1920;
static constexpr int sInitialWindowHeight = 1080;

Application::Application()
    : mSaveData()
    , mWindow(sInitialWindowWidth, sInitialWindowHeight)
    , mInstance()
    , mRenderDevice(mInstance)
    , mSwapchain(mInstance, mRenderDevice)
    , mVulkanImGui(mWindow, mInstance, mRenderDevice, mSwapchain)
    , mRenderer(mRenderDevice, mSaveData)
    , mEditor(mRenderer, mSaveData)
{
}

Application::~Application() = default;

void Application::run()
{
    float currentTime = glfwGetTime();

    while (mWindow.opened())
    {
        float dt = glfwGetTime() - currentTime;
        currentTime = glfwGetTime();

        handleEvents();
        update(dt);
        render();
    }

    vkDeviceWaitIdle(mRenderDevice.device);
}

void Application::recreateSwapchain()
{
    while (mWindow.width() == 0 || mWindow.height() == 0)
        mWindow.waitEvents();

    vkDeviceWaitIdle(mRenderDevice.device);

    mSwapchain.recreate();
    mVulkanImGui.onSwapchainRecreate();
}

void Application::handleEvents()
{
    mWindow.pollEvents();

    for (const Event& event : mWindow.events())
    {
    }
}

void Application::update(float dt)
{
    mVulkanImGui.begin();

    mRenderer.processMainThreadTasks();
    mEditor.update(dt);

    mVulkanImGui.end();
}

void Application::fillCommandBuffer(uint32_t imageIndex)
{
    VkCommandBufferBeginInfo commandBufferBeginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    VkResult result = vkBeginCommandBuffer(mSwapchain.commandBuffer, &commandBufferBeginInfo);
    vulkanCheck(result, "Failed to begin command buffer.");

    mRenderer.render(mSwapchain.commandBuffer);
    mVulkanImGui.render(mSwapchain.commandBuffer, imageIndex);

    vkEndCommandBuffer(mSwapchain.commandBuffer);
}

void Application::render()
{
    vkResetFences(mRenderDevice.device, 1, &mSwapchain.inFlightFence);

    uint32_t swapchainImageIndex;
    VkResult result = vkAcquireNextImageKHR(mRenderDevice.device,
                                            mSwapchain.swapchain,
                                           UINT64_MAX,
                                           mSwapchain.imageReadySemaphore,
                                           VK_NULL_HANDLE,
                                           &swapchainImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        recreateSwapchain();
        return;
    }

    fillCommandBuffer(swapchainImageIndex);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &mSwapchain.imageReadySemaphore,
        .pWaitDstStageMask = &waitStage,
        .commandBufferCount = 1,
        .pCommandBuffers = &mSwapchain.commandBuffer,
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &mSwapchain.renderCompleteSemaphore
    };

    result = vkQueueSubmit(mRenderDevice.graphicsQueue, 1, &submitInfo, mSwapchain.inFlightFence);
    vulkanCheck(result, "Failed queue submit.");

    VkPresentInfoKHR presentInfo {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &mSwapchain.renderCompleteSemaphore,
        .swapchainCount = 1,
        .pSwapchains = &mSwapchain.swapchain,
        .pImageIndices = &swapchainImageIndex,
        .pResults = nullptr
    };

    result = vkQueuePresentKHR(mRenderDevice.graphicsQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        recreateSwapchain();

    vkWaitForFences(mRenderDevice.device, 1, &mSwapchain.inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetCommandBuffer(mSwapchain.commandBuffer, 0);
}
