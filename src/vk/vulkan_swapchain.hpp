//
// Created by Gianni on 3/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_SWAPCHAIN_HPP
#define VULKANRENDERINGENGINE_VULKAN_SWAPCHAIN_HPP

#include <glfw/glfw3.h>
#include <vulkan/vulkan.h>
#include "vulkan_device.hpp"
#include "vulkan_image.hpp"
#include "../utils.hpp"

class VulkanSwapchain
{
public:
    VkSwapchainKHR swapchain;
    VkSurfaceKHR surface;

    VkFormat format;
    VkExtent2D extent;

    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

public:
    void create(GLFWwindow* window,
                const VulkanInstance& instance,
                const VulkanPhysicalDevice& physicalDevice,
                const VulkanDevice& device);
    void destroy(const VulkanInstance& instance, const VulkanDevice& device);

    // todo: resize()

    static constexpr uint32_t swapchainImageCount();

private:
    void createSurface(GLFWwindow* window, const VulkanInstance& instance);
    void createSwapchain(const VulkanPhysicalDevice& physicalDevice, const VulkanDevice& device);
    void createSwapchainImages(const VulkanDevice& device);
    void createCommandPool(const VulkanDevice& device, uint32_t graphicsQueueFamilyIndex);
    void createCommandBuffers(const VulkanDevice& device);
};

#endif //VULKANRENDERINGENGINE_VULKAN_SWAPCHAIN_HPP
