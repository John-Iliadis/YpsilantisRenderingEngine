//
// Created by Gianni on 3/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_SWAPCHAIN_HPP
#define VULKANRENDERINGENGINE_VULKAN_SWAPCHAIN_HPP

#include <glfw/glfw3.h>
#include <vulkan/vulkan.h>
#include "vulkan_render_device.hpp"
#include "vulkan_image.hpp"
#include "vulkan_utils.hpp"

class VulkanSwapchain
{
public:
    VkSwapchainKHR swapchain;
    VkSurfaceKHR surface;

    VkFormat format;
    VkExtent2D extent;

    std::vector<VkImage> images;
    std::vector<VkImageView> imageViews;
    std::vector<VkCommandBuffer> commandBuffers;

public:
    void create(GLFWwindow* window,
                const VulkanInstance& instance,
                const VulkanRenderDevice& renderDevice);
    void destroy(const VulkanInstance& instance, const VulkanRenderDevice& renderDevice);

    // todo: resize()

    static constexpr uint32_t swapchainImageCount();

private:
    void createSurface(GLFWwindow* window, const VulkanInstance& instance);
    void createSwapchain(const VulkanRenderDevice& renderDevice);
    void createSwapchainImages(const VulkanRenderDevice& renderDevice);
    void createCommandBuffers(const VulkanRenderDevice& renderDevice);
};

#endif //VULKANRENDERINGENGINE_VULKAN_SWAPCHAIN_HPP
