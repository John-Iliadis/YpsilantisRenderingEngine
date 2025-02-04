//
// Created by Gianni on 3/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_SWAPCHAIN_HPP
#define VULKANRENDERINGENGINE_VULKAN_SWAPCHAIN_HPP

#include <glm/glm.hpp>
#include "../window/window.hpp"
#include "vulkan_render_device.hpp"
#include "vulkan_image.hpp"
#include "vulkan_utils.hpp"

class VulkanSwapchain
{
public:
    VkSwapchainKHR swapchain;
    VkSurfaceKHR surface;

    VkCommandBuffer commandBuffer;
    VkFence inFlightFence;
    VkSemaphore imageReadySemaphore;
    VkSemaphore renderCompleteSemaphore;

    std::vector<VulkanImage> images;

public:
    VulkanSwapchain(const Window& window, const VulkanInstance& instance, const VulkanRenderDevice& renderDevice);
    ~VulkanSwapchain();

    void recreate();

    VkFormat getFormat() const;
    VkExtent2D getExtent() const;
    uint32_t getImageCount() const;

private:
    void createSurface(const Window& window);
    void createSwapchain();
    void createSwapchainImages();
    void createCommandBuffers();
    void createSyncObjects();

private:
    const VulkanInstance& mInstance;
    const VulkanRenderDevice& mRenderDevice;

    VkFormat mFormat;
    VkExtent2D mExtent;
    uint32_t mImageCount;
};

#endif //VULKANRENDERINGENGINE_VULKAN_SWAPCHAIN_HPP
