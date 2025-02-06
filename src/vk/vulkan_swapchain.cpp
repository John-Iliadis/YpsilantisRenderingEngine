//
// Created by Gianni on 3/01/2025.
//

#include "vulkan_swapchain.hpp"

VulkanSwapchain::VulkanSwapchain(const VulkanInstance &instance, const VulkanRenderDevice &renderDevice)
    : mInstance(instance)
    , mRenderDevice(renderDevice)
{
    createSurface();
    createSwapchain();
    createSwapchainImages();
    createCommandBuffer();
    createSyncObjects();
}

VulkanSwapchain::~VulkanSwapchain()
{
    vkDestroyFence(mRenderDevice.device, inFlightFence, nullptr);
    vkDestroySemaphore(mRenderDevice.device, imageReadySemaphore, nullptr);
    vkDestroySemaphore(mRenderDevice.device, renderCompleteSemaphore, nullptr);

    for (size_t i = 0; i < 2; ++i)
    {
        vkDestroyImageView(mRenderDevice.device, imageViews.at(i), nullptr);
    }

    vkDestroySwapchainKHR(mRenderDevice.device, swapchain, nullptr);
    vkDestroySurfaceKHR(mInstance.instance, surface, nullptr);
}

void VulkanSwapchain::recreate()
{
    // swapchain
    for (size_t i = 0; i < 2; ++i)
        vkDestroyImageView(mRenderDevice.device, imageViews.at(i), nullptr);
    vkDestroySwapchainKHR(mRenderDevice.device, swapchain, nullptr);

    createSwapchain();
    createSwapchainImages();

    // sync objects
    vkDestroyFence(mRenderDevice.device, inFlightFence, nullptr);
    vkDestroySemaphore(mRenderDevice.device, imageReadySemaphore, nullptr);
    vkDestroySemaphore(mRenderDevice.device, renderCompleteSemaphore, nullptr);

    createSyncObjects();
}

void VulkanSwapchain::createSurface()
{
    VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfoKhr {
        .sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
        .hinstance = (HINSTANCE)GetModuleHandle(nullptr),
        .hwnd = getHWND()
    };

    VkResult result = vkCreateWin32SurfaceKHR(mInstance.instance, &win32SurfaceCreateInfoKhr, nullptr, &surface);
    vulkanCheck(result, "Failed to create Vulkan surface.");
}

void VulkanSwapchain::createSwapchain()
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mRenderDevice.physicalDevice, surface, &surfaceCapabilities);

    mFormat = VK_FORMAT_R8G8B8A8_UNORM;
    mExtent = surfaceCapabilities.currentExtent;

    VkSwapchainCreateInfoKHR swapchainCreateInfoKhr {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = 2,
        .imageFormat = mFormat,
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = mExtent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    VkResult result = vkCreateSwapchainKHR(mRenderDevice.device, &swapchainCreateInfoKhr, nullptr, &swapchain);
    vulkanCheck(result, "Failed to create swapchain.");
}

void VulkanSwapchain::createSwapchainImages()
{
    uint32_t imageCount = 2;
    vkGetSwapchainImagesKHR(mRenderDevice.device, swapchain, &imageCount, images.data());

    for (size_t i = 0; i < imageCount; ++i)
    {
        imageViews.at(i) = createImageView(mRenderDevice,
                                           images.at(i),
                                           VK_IMAGE_VIEW_TYPE_2D,
                                           mFormat,
                                           VK_IMAGE_ASPECT_COLOR_BIT);

        setVulkanObjectDebugName(mRenderDevice,
                                 VK_OBJECT_TYPE_IMAGE,
                                 std::format("VulkanSwapchain::images.at({})", i),
                                 images.at(i));

        setVulkanObjectDebugName(mRenderDevice,
                                 VK_OBJECT_TYPE_IMAGE_VIEW,
                                 std::format("VulkanSwapchain::imageViews.at({})", i),
                                 imageViews.at(i));
    }
}

void VulkanSwapchain::createCommandBuffer()
{
    VkCommandBufferAllocateInfo commandBufferAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = mRenderDevice.commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    VkResult result = vkAllocateCommandBuffers(mRenderDevice.device, &commandBufferAllocateInfo, &commandBuffer);
    vulkanCheck(result, "Failed to allocate command buffers.");

    setVulkanObjectDebugName(mRenderDevice,
                             VK_OBJECT_TYPE_COMMAND_BUFFER,
                             std::format("VulkanSwapchain::commandBuffer"),
                             commandBuffer);
}

void VulkanSwapchain::createSyncObjects()
{
    inFlightFence = createFence(mRenderDevice, true, "VulkanSwapchain::inFlightFence");
    imageReadySemaphore = createSemaphore(mRenderDevice, "VulkanSwapchain::imageReadySemaphore");
    renderCompleteSemaphore = createSemaphore(mRenderDevice, "VulkanSwapchain::renderCompleteSemaphore");
}

VkFormat VulkanSwapchain::getFormat() const
{
    return mFormat;
}

VkExtent2D VulkanSwapchain::getExtent() const
{
    return mExtent;
}
