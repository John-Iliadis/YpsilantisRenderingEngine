//
// Created by Gianni on 3/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_SWAPCHAIN_HPP
#define VULKANRENDERINGENGINE_VULKAN_SWAPCHAIN_HPP

#include <glfw/glfw3.h>
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
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
    uint32_t imageCount;

    std::vector<VulkanImage> images;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkFence> inFlightFences;
    std::vector<VkSemaphore> imageReadySemaphores;
    std::vector<VkSemaphore> renderCompleteSemaphores;

public:
    void create(GLFWwindow* window,
                const VulkanInstance& instance,
                const VulkanRenderDevice& renderDevice);
    void destroy(const VulkanInstance& instance, const VulkanRenderDevice& renderDevice);
    void recreate(const VulkanRenderDevice& renderDevice);

private:
    void createSurface(GLFWwindow* window, const VulkanInstance& instance);
    void createSwapchain(const VulkanRenderDevice& renderDevice);
    void createSwapchainImages(const VulkanRenderDevice& renderDevice);
    void createCommandBuffers(const VulkanRenderDevice& renderDevice);
    void createSyncObjects(const VulkanRenderDevice& renderDevice);
};

#endif //VULKANRENDERINGENGINE_VULKAN_SWAPCHAIN_HPP
