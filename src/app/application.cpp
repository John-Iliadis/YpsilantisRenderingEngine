//
// Created by Gianni on 2/01/2025.
//

#include "application.hpp"

static constexpr int sInitialWindowWidth = 1920;
static constexpr int sInitialWindowHeight = 1080;

Application::Application()
    : mWindow(sInitialWindowWidth, sInitialWindowHeight)
    , mInstance()
    , mRenderDevice(mInstance)
    , mSwapchain(mInstance, mRenderDevice)
    , mRenderer(mRenderDevice)
    , mEditor(mRenderer)
{
    createRenderPass();
    createSwapchainFramebuffers();

    mVulkanImGui.init(mWindow, mInstance, mRenderDevice, mRenderPass);
}

Application::~Application()
{
    vkDestroyRenderPass(mRenderDevice.device, mRenderPass, nullptr);
    for (uint32_t i = 0; i < mSwapchainFramebuffers.size(); ++i)
        vkDestroyFramebuffer(mRenderDevice.device, mSwapchainFramebuffers.at(i), nullptr);
    mVulkanImGui.terminate();
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

    mSwapchain.recreate();

    // swapchain framebuffers
    for (uint32_t i = 0; i < mSwapchainFramebuffers.size(); ++i)
        vkDestroyFramebuffer(mRenderDevice.device, mSwapchainFramebuffers.at(i), nullptr);
    createSwapchainFramebuffers();
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
    vkResetCommandBuffer(mSwapchain.commandBuffer, 0);
    mRenderer.releaseResources();

    VkCommandBufferBeginInfo commandBufferBeginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr
    };

    VkResult result = vkBeginCommandBuffer(mSwapchain.commandBuffer, &commandBufferBeginInfo);
    vulkanCheck(result, "Failed to begin command buffer.");

    VkClearValue clearValue {{0.2f, 0.2f, 0.2f, 0.2f}};

    VkRenderPassBeginInfo renderPassBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mRenderPass,
        .framebuffer = mSwapchainFramebuffers.at(imageIndex),
        .renderArea = {
            .offset = {0, 0},
            .extent = mSwapchain.getExtent()
        },
        .clearValueCount = 1,
        .pClearValues = &clearValue
    };

    vkCmdBeginRenderPass(mSwapchain.commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    mVulkanImGui.render(mSwapchain.commandBuffer);
    vkCmdEndRenderPass(mSwapchain.commandBuffer);
    vkEndCommandBuffer(mSwapchain.commandBuffer);
}

void Application::render()
{
    vkWaitForFences(mRenderDevice.device, 1, &mSwapchain.inFlightFence, VK_TRUE, UINT64_MAX);
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
}

void Application::createRenderPass()
{
    VkAttachmentDescription attachmentDescription {
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference attachmentReference {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpassDescription {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .colorAttachmentCount = 1,
        .pColorAttachments = &attachmentReference
    };

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &attachmentDescription,
        .subpassCount = 1,
        .pSubpasses = &subpassDescription
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mRenderPass);
    vulkanCheck(result, "Failed to create renderpass");
}

void Application::createSwapchainFramebuffers()
{
    for (uint32_t i = 0; i < mSwapchainFramebuffers.size(); ++i)
    {
        VkFramebufferCreateInfo framebufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = mRenderPass,
            .attachmentCount = 1,
            .pAttachments = &mSwapchain.imageViews.at(i),
            .width = mSwapchain.getExtent().width,
            .height = mSwapchain.getExtent().height,
            .layers = 1
        };

        VkResult result = vkCreateFramebuffer(mRenderDevice.device,
                                              &framebufferCreateInfo,
                                              nullptr,
                                              &mSwapchainFramebuffers.at(i));
        vulkanCheck(result, "Failed to create swapchain framebuffer");

        setVulkanObjectDebugName(mRenderDevice,
                                 VK_OBJECT_TYPE_FRAMEBUFFER,
                                 std::format("Application::mSwapchainFramebuffers.at({})", i),
                                 mSwapchainFramebuffers.at(i));
    }
}
