//
// Created by Gianni on 3/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_SWAPCHAIN_HPP
#define VULKANRENDERINGENGINE_VULKAN_SWAPCHAIN_HPP

#include <glm/glm.hpp>
#include "vulkan_render_device.hpp"
#include "vulkan_image.hpp"
#include "vulkan_utils.hpp"
#include "../utils/utils.hpp"

class VulkanSwapchain
{
public:
    VkSwapchainKHR swapchain;
    VkSurfaceKHR surface;

    VkCommandBuffer commandBuffer;

    VkFence inFlightFence;
    VkSemaphore imageReadySemaphore;
    VkSemaphore renderCompleteSemaphore;

    std::array<VkImage, 2> images;
    std::array<VkImageView, 2> imageViews;

public:
    VulkanSwapchain(const VulkanInstance& instance, const VulkanRenderDevice& renderDevice);
    ~VulkanSwapchain();

    void recreate();

    VkFormat getFormat() const;
    VkExtent2D getExtent() const;

private:
    void createSurface();
    void createSwapchain();
    void createSwapchainImages();
    void createCommandBuffer();
    void createSyncObjects();

private:
    const VulkanInstance& mInstance;
    const VulkanRenderDevice& mRenderDevice;

    VkFormat mFormat;
    VkExtent2D mExtent;
};

#endif //VULKANRENDERINGENGINE_VULKAN_SWAPCHAIN_HPP
