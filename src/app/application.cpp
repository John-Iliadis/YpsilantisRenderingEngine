//
// Created by Gianni on 2/01/2025.
//

#include "application.hpp"

static constexpr int sInitialWindowWidth = 1920;
static constexpr int sInitialWindowHeight = 1080;

Application::Application()
    : mWindow(sInitialWindowWidth, sInitialWindowHeight)
{
    mInstance.create();
    mRenderDevice.create(mInstance);
    mSwapchain.create(mWindow, mInstance, mRenderDevice);
    mRenderer.init(mWindow, mInstance, mRenderDevice, mSwapchain);
}

Application::~Application()
{
    mRenderer.terminate();
    mSwapchain.destroy(mInstance, mRenderDevice);
    mRenderDevice.destroy();
    mInstance.destroy();
}

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

    mSwapchain.recreate(mRenderDevice);
    mRenderer.onSwapchainRecreate();
}

void Application::handleEvents()
{
    mWindow.pollEvents();

    for (const Event& event : mWindow.events())
        mRenderer.handleEvent(event);
}

void Application::update(float dt)
{
    countFPS(dt);
    mRenderer.update(dt);
}

void Application::fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t frameIndex, uint32_t swapchainImageIndex)
{
    VkCommandBufferBeginInfo commandBufferBeginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr
    };

    VkResult result = vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
    vulkanCheck(result, "Failed to begin command buffer.");

    mRenderer.fillCommandBuffer(commandBuffer, frameIndex, swapchainImageIndex);

    vkEndCommandBuffer(commandBuffer);
}

void Application::render()
{
    static uint32_t currentFrame = 0;

    vkWaitForFences(mRenderDevice.device, 1, &mSwapchain.inFlightFence.at(currentFrame), VK_TRUE, UINT64_MAX);
    vkResetFences(mRenderDevice.device, 1, &mSwapchain.inFlightFence.at(currentFrame));

    uint32_t swapchainImageIndex;
    VkResult result = vkAcquireNextImageKHR(mRenderDevice.device,
                                            mSwapchain.swapchain,
                                           UINT64_MAX,
                                           mSwapchain.imageReadySemaphore.at(currentFrame),
                                           VK_NULL_HANDLE,
                                           &swapchainImageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
        recreateSwapchain();
        return;
    }

    fillCommandBuffer(mSwapchain.commandBuffer.at(currentFrame), currentFrame, swapchainImageIndex);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &mSwapchain.imageReadySemaphore.at(currentFrame),
        .pWaitDstStageMask = &waitStage,
        .commandBufferCount = 1,
        .pCommandBuffers = &mSwapchain.commandBuffer.at(currentFrame),
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &mSwapchain.renderCompleteSemaphore.at(currentFrame)
    };

    result = vkQueueSubmit(mRenderDevice.graphicsQueue,
                           1, &submitInfo,
                           mSwapchain.inFlightFence.at(currentFrame));
    vulkanCheck(result, "Failed queue submit.");

    VkPresentInfoKHR presentInfo {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &mSwapchain.renderCompleteSemaphore.at(currentFrame),
        .swapchainCount = 1,
        .pSwapchains = &mSwapchain.swapchain,
        .pImageIndices = &swapchainImageIndex,
        .pResults = nullptr
    };

    result = vkQueuePresentKHR(mRenderDevice.graphicsQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        recreateSwapchain();

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void Application::countFPS(float dt)
{
    static float frameCount = 0;
    static float accumulatedTime = 0.f;

    ++frameCount;
    accumulatedTime += dt;

    if (accumulatedTime >= 1.f)
    {
        debugLog(std::format("FPS: {}", frameCount));

        frameCount = 0;
        accumulatedTime = 0.f;
    }
}
