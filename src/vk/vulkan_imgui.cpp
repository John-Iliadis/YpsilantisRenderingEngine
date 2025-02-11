//
// Created by Gianni on 8/01/2025.
//

#include "vulkan_imgui.hpp"

VulkanImGui::VulkanImGui(const Window &window,
                         const VulkanInstance &instance,
                         const VulkanRenderDevice &renderDevice,
                         const VulkanSwapchain &swapchain)
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
        .QueueFamily = renderDevice.getGraphicsQueueFamilyIndex(),
        .Queue = renderDevice.graphicsQueue,
        .DescriptorPool = renderDevice.descriptorPool,
        .RenderPass = swapchain.renderPass,
        .MinImageCount = 2,
        .ImageCount = 2,
        .MSAASamples = VK_SAMPLE_COUNT_1_BIT
    };

    ImGui_ImplGlfw_InitForVulkan(window, true);
    ImGui_ImplVulkan_Init(&initInfo);

    auto& style = ImGui::GetStyle();
    style.WindowMenuButtonPosition = ImGuiDir_Right;
    style.ColorButtonPosition = ImGuiDir_Left;
    style.ChildRounding = 6.f;
    style.GrabRounding = 3.f;
}

VulkanImGui::~VulkanImGui()
{
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void VulkanImGui::begin()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport();
}

void VulkanImGui::end()
{
    ImGui::Render();
}

void VulkanImGui::render(VkCommandBuffer commandBuffer)
{
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
}
