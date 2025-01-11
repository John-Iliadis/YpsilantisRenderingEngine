//
// Created by Gianni on 3/01/2025.
//

#include "vulkan_swapchain.hpp"

void VulkanSwapchain::create(GLFWwindow* window,
                             const VulkanInstance& instance,
                             const VulkanRenderDevice& renderDevice)
{
    createSurface(window, instance);
    createSwapchain(renderDevice);
    createSwapchainImages(renderDevice);
    createCommandBuffers(renderDevice);
    createSyncObjects(renderDevice);
}

void VulkanSwapchain::destroy(const VulkanInstance& instance, const VulkanRenderDevice& renderDevice)
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        vkDestroyFence(renderDevice.device, inFlightFences.at(i), nullptr);
        vkDestroySemaphore(renderDevice.device, imageReadySemaphores.at(i), nullptr);
        vkDestroySemaphore(renderDevice.device, renderCompleteSemaphores.at(i), nullptr);
    }

    for (size_t i = 0; i < imageCount; ++i)
    {
        vkDestroyImageView(renderDevice.device, images.at(i).imageView, nullptr);
    }

    vkDestroySwapchainKHR(renderDevice.device, swapchain, nullptr);
    vkDestroySurfaceKHR(instance.instance, surface, nullptr);
}

void VulkanSwapchain::recreate(const VulkanRenderDevice& renderDevice)
{
    for (size_t i = 0; i < imageCount; ++i)
        vkDestroyImageView(renderDevice.device, images.at(i).imageView, nullptr);
    vkDestroySwapchainKHR(renderDevice.device, swapchain, nullptr);

    createSwapchain(renderDevice);
    createSwapchainImages(renderDevice);
}

void VulkanSwapchain::createSurface(GLFWwindow *window, const VulkanInstance &instance)
{
    VkResult result = glfwCreateWindowSurface(instance.instance, window, nullptr, &surface);
    vulkanCheck(result, "Failed to create Vulkan surface.");
}

void VulkanSwapchain::createSwapchain(const VulkanRenderDevice& renderDevice)
{
    VkSurfaceCapabilitiesKHR surfaceCapabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(renderDevice.physicalDevice, surface, &surfaceCapabilities);

    format = VK_FORMAT_R8G8B8A8_UNORM;
    extent = surfaceCapabilities.currentExtent;
    imageCount = glm::max(glm::min(3u, surfaceCapabilities.maxImageCount), surfaceCapabilities.minImageCount);

    VkSwapchainCreateInfoKHR swapchainCreateInfoKhr {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = surface,
        .minImageCount = imageCount,
        .imageFormat = format,
        .imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR,
        .imageExtent = extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .preTransform = surfaceCapabilities.currentTransform,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE
    };

    VkResult result = vkCreateSwapchainKHR(renderDevice.device, &swapchainCreateInfoKhr, nullptr, &swapchain);
    vulkanCheck(result, "Failed to create swapchain.");
}

void VulkanSwapchain::createSwapchainImages(const VulkanRenderDevice& renderDevice)
{
    images.resize(imageCount);

    VkImage swapchainImages[imageCount];
    vkGetSwapchainImagesKHR(renderDevice.device, swapchain, &imageCount, swapchainImages);

    for (size_t i = 0; i < imageCount; ++i)
    {
        images.at(i).image = swapchainImages[i];
        images.at(i).imageView = createImageView(renderDevice,
                                                    images.at(i).image,
                                                    VK_IMAGE_VIEW_TYPE_2D,
                                                    format,
                                                    VK_IMAGE_ASPECT_COLOR_BIT);

        setImageDebugName(renderDevice, images.at(i), "Swapchain", i);
    }
}

void VulkanSwapchain::createCommandBuffers(const VulkanRenderDevice& renderDevice)
{
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo commandBufferAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = renderDevice.commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = MAX_FRAMES_IN_FLIGHT
    };

    VkResult result = vkAllocateCommandBuffers(renderDevice.device, &commandBufferAllocateInfo, commandBuffers.data());
    vulkanCheck(result, "Failed to allocate command buffers.");

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        setDebugVulkanObjectName(renderDevice.device,
                                 VK_OBJECT_TYPE_COMMAND_BUFFER,
                                 std::format("Swapchain command buffer {}", i),
                                 commandBuffers.at(i));
    }
}

void VulkanSwapchain::createSyncObjects(const VulkanRenderDevice &renderDevice)
{
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    imageReadySemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderCompleteSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        inFlightFences.at(i) = createFence(renderDevice.device, true, std::format("Swapchain in flight fence {}", i).c_str());
        imageReadySemaphores.at(i) = createSemaphore(renderDevice.device, std::format("Swapchain image ready semaphore {}", i).c_str());
        renderCompleteSemaphores.at(i) = createSemaphore(renderDevice.device, std::format("Swapchain render complete semaphore {}", i).c_str());
    }
}
