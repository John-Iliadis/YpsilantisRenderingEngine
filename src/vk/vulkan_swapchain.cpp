//
// Created by Gianni on 3/01/2025.
//

#include "vulkan_swapchain.hpp"

void VulkanSwapchain::create(GLFWwindow* window,
                             const VulkanInstance& instance,
                             const VulkanPhysicalDevice& physicalDevice,
                             const VulkanDevice& device)
{
    createSurface(window, instance);
    createSwapchain(physicalDevice, device);
    createSwapchainImages(device);
    createCommandPool(device, physicalDevice.graphicsQueueFamilyIndex);
    createCommandBuffers(device);
}

void VulkanSwapchain::destroy(const VulkanInstance &instance, const VulkanDevice &device)
{
    vkDestroyCommandPool(device.device, commandPool, nullptr);

    for (size_t i = 0; i < swapchainImageCount(); ++i)
    {
        vkDestroyImageView(device.device, imageViews.at(i), nullptr);
    }

    vkDestroySwapchainKHR(device.device, swapchain, nullptr);
    vkDestroySurfaceKHR(instance.instance, surface, nullptr);
}

void VulkanSwapchain::createSurface(GLFWwindow *window, const VulkanInstance &instance)
{
    VkResult result = glfwCreateWindowSurface(instance.instance, window, nullptr, &surface);
    vulkanCheck(result, "Failed to create Vulkan surface.");
}

void VulkanSwapchain::createSwapchain(const VulkanPhysicalDevice& physicalDevice, const VulkanDevice& device)
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice.physicalDevice, surface, &surfaceCapabilities);

    format = VK_FORMAT_R8G8B8A8_UNORM;
    extent = surfaceCapabilities.currentExtent;

    VkSwapchainCreateInfoKHR swapchainCreateInfoKhr {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = swapchainImageCount(),
        .imageFormat = format,
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = extent,
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

    VkResult result = vkCreateSwapchainKHR(device.device, &swapchainCreateInfoKhr, nullptr, &swapchain);
    vulkanCheck(result, "Failed to create swapchain.");
}

void VulkanSwapchain::createSwapchainImages(const VulkanDevice &device)
{
    images.resize(swapchainImageCount());
    imageViews.resize(swapchainImageCount());

    uint32_t imgCount = swapchainImageCount();
    vkGetSwapchainImagesKHR(device.device, swapchain, &imgCount, images.data());

    for (size_t i = 0; i < swapchainImageCount(); ++i)
    {
        imageViews.at(i) = createImageView(device,
                                           images.at(i),
                                           VK_IMAGE_VIEW_TYPE_2D,
                                           format,
                                           VK_IMAGE_ASPECT_COLOR_BIT);

        setDebugVulkanObjectName(device.device,
                                 VK_OBJECT_TYPE_IMAGE_VIEW,
                                 std::format("Swapchain image view {}", i),
                                 &imageViews.at(i));
    }
}

void VulkanSwapchain::createCommandPool(const VulkanDevice &device, uint32_t graphicsQueueFamilyIndex)
{
    VkCommandPoolCreateInfo commandPoolCreateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = graphicsQueueFamilyIndex
    };

    VkResult result = vkCreateCommandPool(device.device, &commandPoolCreateInfo, nullptr, &commandPool);
    vulkanCheck(result, "Failed to create command pool.");
}

void VulkanSwapchain::createCommandBuffers(const VulkanDevice &device)
{
    commandBuffers.resize(swapchainImageCount());
    
    VkCommandBufferAllocateInfo commandBufferAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = swapchainImageCount()
    };
    
    VkResult result = vkAllocateCommandBuffers(device.device, &commandBufferAllocateInfo, commandBuffers.data());
    vulkanCheck(result, "Failed to allocate command buffers.");

    for (uint32_t i = 0; i < swapchainImageCount(); ++i)
    {
        setDebugVulkanObjectName(device.device,
                                 VK_OBJECT_TYPE_COMMAND_BUFFER,
                                 std::format("Swapchain command buffer {}", i),
                                 commandBuffers.at(i));
    }
}

constexpr uint32_t VulkanSwapchain::swapchainImageCount()
{
    return 3;
}
