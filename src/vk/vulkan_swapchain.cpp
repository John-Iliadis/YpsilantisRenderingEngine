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
    createRenderpass();
    createFramebuffers();
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
        vkDestroyFramebuffer(mRenderDevice.device, framebuffers.at(i), nullptr);
        vkDestroyImageView(mRenderDevice.device, imageViews.at(i), nullptr);
    }

    vkDestroyRenderPass(mRenderDevice.device, renderPass, nullptr);
    vkDestroySwapchainKHR(mRenderDevice.device, swapchain, nullptr);
    vkDestroySurfaceKHR(mInstance.instance, surface, nullptr);
}

void VulkanSwapchain::recreate()
{
    // swapchain
    for (size_t i = 0; i < 2; ++i)
    {
        vkDestroyFramebuffer(mRenderDevice.device, framebuffers.at(i), nullptr);
        vkDestroyImageView(mRenderDevice.device, imageViews.at(i), nullptr);
    }

    vkDestroySwapchainKHR(mRenderDevice.device, swapchain, nullptr);

    createSwapchain();
    createSwapchainImages();
    createFramebuffers();

    // sync objects
    vkDestroyFence(mRenderDevice.device, inFlightFence, nullptr);
    vkDestroySemaphore(mRenderDevice.device, imageReadySemaphore, nullptr);
    vkDestroySemaphore(mRenderDevice.device, renderCompleteSemaphore, nullptr);

    createSyncObjects();
}

VkFormat VulkanSwapchain::getFormat() const
{
    return mFormat;
}

VkExtent2D VulkanSwapchain::getExtent() const
{
    return mExtent;
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
    inFlightFence = createFence(mRenderDevice, false, "VulkanSwapchain::inFlightFence");
    imageReadySemaphore = createSemaphore(mRenderDevice, "VulkanSwapchain::imageReadySemaphore");
    renderCompleteSemaphore = createSemaphore(mRenderDevice, "VulkanSwapchain::renderCompleteSemaphore");
}

void VulkanSwapchain::createRenderpass()
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

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &renderPass);
    vulkanCheck(result, "Failed to create renderpass");

    setVulkanObjectDebugName(mRenderDevice,
                             VK_OBJECT_TYPE_RENDER_PASS,
                             "VulkanSwapchain::renderPass",
                             renderPass);
}

void VulkanSwapchain::createFramebuffers()
{
    for (uint32_t i = 0; i < 2; ++i)
    {
        VkFramebufferCreateInfo framebufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = renderPass,
            .attachmentCount = 1,
            .pAttachments = &imageViews.at(i),
            .width = mExtent.width,
            .height = mExtent.height,
            .layers = 1
        };

        VkResult result = vkCreateFramebuffer(mRenderDevice.device,
                                              &framebufferCreateInfo,
                                              nullptr,
                                              &framebuffers.at(i));
        vulkanCheck(result, "Failed to create swapchain framebuffer");

        setVulkanObjectDebugName(mRenderDevice,
                                 VK_OBJECT_TYPE_FRAMEBUFFER,
                                 std::format("VulkanSwapchain::framebuffers.at({})", i),
                                 framebuffers.at(i));
    }
}
