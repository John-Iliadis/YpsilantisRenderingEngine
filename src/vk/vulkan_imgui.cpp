//
// Created by Gianni on 8/01/2025.
//

#include "vulkan_imgui.hpp"

void initImGui(GLFWwindow* window,
               const VulkanInstance& instance,
               const VulkanRenderDevice& renderDevice,
               VkRenderPass renderPass)
{
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplVulkan_InitInfo initInfo {
        .Instance = instance.instance,
        .PhysicalDevice = renderDevice.physicalDevice,
        .Device = renderDevice.device,
        .QueueFamily = renderDevice.mGraphicsQueueFamilyIndex,
        .Queue = renderDevice.graphicsQueue,
        .DescriptorPool = renderDevice.descriptorPool,
        .RenderPass = renderPass,
        .MinImageCount = MAX_FRAMES_IN_FLIGHT,
        .ImageCount = MAX_FRAMES_IN_FLIGHT,
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT
    };

    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_Init(&initInfo);
}

void terminateImGui()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void beginImGui()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport(nullptr, ImGuiDockNodeFlags_PassthruCentralNode);
}

void endImGui()
{
    ImGui::Render();
}

void imGuiFillCommandBuffer(VkCommandBuffer commandBuffer)
{
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}
