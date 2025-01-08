//
// Created by Gianni on 2/01/2025.
//

#include "application.hpp"

static constexpr int sInitialWindowWidth = 1920;
static constexpr int sInitialWindowHeight = 1080;

Application::Application()
    : mWindow(sInitialWindowWidth, sInitialWindowHeight)
    , mInstance()
    , mRenderDevice()
    , mSwapchain()
    , mViewProjectionDSLayout()
    , mCurrentFrame()
    , mSceneCamera(glm::vec3(0.f, 0.f, -5.f), 30.f, sInitialWindowHeight, sInitialWindowHeight)
{
    initializeGLFW();
    initializeVulkan();

    mCubeRenderer.init(mRenderDevice,
                       &mSwapchain.images,
                       &mDepthImages,
                       mSwapchain.extent,
                       mViewProjectionDSLayout,
                       mViewProjectionDS);

    mRenderFinish.init(mRenderDevice, &mSwapchain.images, mSwapchain.extent);
}

Application::~Application()
{
    mRenderFinish.terminate();
    mCubeRenderer.terminate();

    for (size_t i = 0; i < VulkanSwapchain::swapchainImageCount(); ++i)
    {
        destroyImage(mRenderDevice, mDepthImages.at(i));
        destroyBuffer(mRenderDevice, mViewProjectionUBOs.at(i));
    }

    vkDestroyDescriptorSetLayout(mRenderDevice.device, mViewProjectionDSLayout, nullptr);
    vkDestroyDescriptorPool(mRenderDevice.device, mDescriptorPool, nullptr);
    mSwapchain.destroy(mInstance, mRenderDevice);
    mRenderDevice.destroy();
    mInstance.destroy();
    glfwTerminate();
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

void Application::initializeGLFW()
{

}

void Application::initializeVulkan()
{
    mInstance.create();
    mRenderDevice.create(mInstance);
    mSwapchain.create(mWindow, mInstance, mRenderDevice);
    createDepthImages();
    createDescriptorPool();
    createViewProjUBOs();
    createViewProjDescriptors();
}

void Application::createDepthImages()
{
    mDepthImages.resize(VulkanSwapchain::swapchainImageCount());

    for (uint32_t i = 0; i < mDepthImages.size(); ++i)
    {
        mDepthImages.at(i) = createImage2D(mRenderDevice,
                                           VK_FORMAT_D32_SFLOAT,
                                           mSwapchain.extent.width,
                                           mSwapchain.extent.height,
                                           VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                           VK_IMAGE_ASPECT_DEPTH_BIT);

        setDebugVulkanObjectName(mRenderDevice.device,
                                 VK_OBJECT_TYPE_IMAGE,
                                 std::format("Application depth image {}", i),
                                 mDepthImages.at(i).image);
    }
}

void Application::createDescriptorPool()
{
    mDescriptorPool = ::createDescriptorPool(mRenderDevice, 1000, 100, 100, 100);

    setDebugVulkanObjectName(mRenderDevice.device,
                             VK_OBJECT_TYPE_DESCRIPTOR_POOL,
                             "Application descriptor pool",
                             mDescriptorPool);


}

void Application::createViewProjUBOs()
{
    mViewProjectionUBOs.resize(VulkanSwapchain::swapchainImageCount());

    VkBufferUsageFlags usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    VkMemoryPropertyFlags memoryProperties {
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    };

    for (size_t i = 0; i < mViewProjectionUBOs.size(); ++i)
    {
        mViewProjectionUBOs.at(i) = createBuffer(mRenderDevice, sizeof(glm::mat4), usage, memoryProperties);

        setDebugVulkanObjectName(mRenderDevice.device,
                                 VK_OBJECT_TYPE_BUFFER,
                                 std::format("ViewProjection buffer {}", i),
                                 mViewProjectionUBOs.at(i).buffer);
    }
}

void Application::createViewProjDescriptors()
{
    mViewProjectionDS.resize(VulkanSwapchain::swapchainImageCount());

    DescriptorSetLayoutCreator DSLayoutCreator(mRenderDevice.device);
    DSLayoutCreator.addLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
    mViewProjectionDSLayout = DSLayoutCreator.create();

    setDebugVulkanObjectName(mRenderDevice.device,
                             VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                             "ViewProjection descriptor set layout",
                             mViewProjectionDSLayout);

    for (uint32_t i = 0; i < mViewProjectionDS.size(); ++i)
    {
        DescriptorSetCreator descriptorSetCreator(mRenderDevice.device, mDescriptorPool, mViewProjectionDSLayout);
        descriptorSetCreator.addBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                       0, mViewProjectionUBOs.at(i).buffer,
                                       0, sizeof(glm::mat4));
        mViewProjectionDS.at(i) = descriptorSetCreator.create();

        setDebugVulkanObjectName(mRenderDevice.device,
                                 VK_OBJECT_TYPE_DESCRIPTOR_SET,
                                 std::format("ViewProjection descriptor set {}", i),
                                 mViewProjectionDS.at(i));
    }
}

void Application::updateUniformBuffers()
{
    mapBufferMemory(mRenderDevice,
                    mViewProjectionUBOs.at(mCurrentFrame),
                    0, sizeof(glm::mat4),
                    glm::value_ptr(mSceneCamera.viewProjection()));
}

void Application::recreateSwapchain()
{
    while (mWindow.width() == 0 || mWindow.height() == 0)
        mWindow.waitEvents();

    vkDeviceWaitIdle(mRenderDevice.device);

    mSwapchain.recreate(mRenderDevice);

    for (size_t i = 0; i < VulkanSwapchain::swapchainImageCount(); ++i)
    {
        destroyImage(mRenderDevice, mDepthImages.at(i));
    }

    createDepthImages();
    mCubeRenderer.onSwapchainRecreate(mSwapchain.extent);
    mRenderFinish.onSwapchainRecreate(mSwapchain.extent);
}

void Application::handleEvents()
{
    mWindow.pollEvents();

    for (const Event& event : mWindow.events())
    {
        mSceneCamera.handleEvent(event);
    }
}

void Application::update(float dt)
{
    countFPS(dt);
    mSceneCamera.update(dt);
    updateUniformBuffers();
}

void Application::fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkCommandBufferBeginInfo commandBufferBeginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr
    };

    VkResult result = vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
    vulkanCheck(result, "Failed to begin command buffer.");

    mCubeRenderer.fillCommandBuffer(commandBuffer, imageIndex);
    mRenderFinish.fillCommandBuffer(commandBuffer, imageIndex);

    vkEndCommandBuffer(commandBuffer);
}

void Application::render()
{
    vkWaitForFences(mRenderDevice.device, 1, &mSwapchain.inFlightFences.at(mCurrentFrame), VK_TRUE, UINT64_MAX);

    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(mRenderDevice.device,
                                            mSwapchain.swapchain,
                                           UINT64_MAX,
                                           mSwapchain.imageReadySemaphores.at(mCurrentFrame),
                                           VK_NULL_HANDLE,
                                           &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
        recreateSwapchain();
        return;
    }

    vkResetFences(mRenderDevice.device, 1, &mSwapchain.inFlightFences.at(mCurrentFrame));

    fillCommandBuffer(mSwapchain.commandBuffers.at(mCurrentFrame), imageIndex);

    VkPipelineStageFlags waitStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &mSwapchain.imageReadySemaphores.at(mCurrentFrame),
        .pWaitDstStageMask = &waitStage,
        .commandBufferCount = 1,
        .pCommandBuffers = &mSwapchain.commandBuffers.at(mCurrentFrame),
        .signalSemaphoreCount = 1,
        .pSignalSemaphores = &mSwapchain.renderCompleteSemaphores.at(mCurrentFrame)
    };

    result = vkQueueSubmit(mRenderDevice.graphicsQueue,
                           1, &submitInfo,
                           mSwapchain.inFlightFences.at(mCurrentFrame));
    vulkanCheck(result, "Failed queue submit.");

    VkPresentInfoKHR presentInfo {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .waitSemaphoreCount = 1,
        .pWaitSemaphores = &mSwapchain.renderCompleteSemaphores.at(mCurrentFrame),
        .swapchainCount = 1,
        .pSwapchains = &mSwapchain.swapchain,
        .pImageIndices = &imageIndex,
        .pResults = nullptr
    };

    result = vkQueuePresentKHR(mRenderDevice.graphicsQueue, &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
        recreateSwapchain();

    mCurrentFrame = (mCurrentFrame + 1) % VulkanSwapchain::swapchainImageCount();
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
