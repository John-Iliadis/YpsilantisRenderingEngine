//
// Created by Gianni on 8/01/2025.
//

#include "vulkan_imgui.hpp"

VulkanImGui::VulkanImGui(const Window &window,
                         const VulkanInstance &instance,
                         const VulkanRenderDevice &renderDevice,
                         const VulkanSwapchain &swapchain)
    : mRenderDevice(renderDevice)
    , mSwapchain(swapchain)
{
    createRenderpass();
    createFramebuffers();

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
        .RenderPass = mRenderpass,
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
    for (uint32_t i = 0; i < 2; ++i)
    {
        vkDestroyFramebuffer(mRenderDevice.device, mFramebuffers.at(i), nullptr);
    }

    vkDestroyRenderPass(mRenderDevice.device, mRenderpass, nullptr);

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

void VulkanImGui::onSwapchainRecreate()
{
    for (uint32_t i = 0; i < 2; ++i)
    {
        vkDestroyFramebuffer(mRenderDevice.device, mFramebuffers.at(i), nullptr);
    }

    createFramebuffers();
}

void VulkanImGui::render(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkClearValue clearValue {{0.2f, 0.2f, 0.2f, 0.2f}};

    VkRenderPassBeginInfo renderPassBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mRenderpass,
        .framebuffer = mFramebuffers.at(imageIndex),
        .renderArea = {
            .offset = {0, 0},
            .extent = mSwapchain.getExtent()
        },
        .clearValueCount = 1,
        .pClearValues = &clearValue
    };

    beginDebugLabel(commandBuffer, "ImGui");
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);
    vkCmdEndRenderPass(commandBuffer);
    endDebugLabel(commandBuffer);
}

void VulkanImGui::createRenderpass()
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

    VkSubpassDependency dependency {
        .srcSubpass = VK_SUBPASS_EXTERNAL,
        .dstSubpass = 0,
        .srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
        .srcAccessMask = 0,
        .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
    };

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &attachmentDescription,
        .subpassCount = 1,
        .pSubpasses = &subpassDescription,
        .dependencyCount = 1,
        .pDependencies = &dependency
    };

    VkResult result = vkCreateRenderPass(mRenderDevice.device, &renderPassCreateInfo, nullptr, &mRenderpass);
    vulkanCheck(result, "Failed to create renderpass");

    setVulkanObjectDebugName(mRenderDevice,
                             VK_OBJECT_TYPE_RENDER_PASS,
                             "VulkanImGui::mRenderPass",
                             mRenderpass);
}

void VulkanImGui::createFramebuffers()
{
    for (uint32_t i = 0; i < 2; ++i)
    {
        VkFramebufferCreateInfo framebufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = mRenderpass,
            .attachmentCount = 1,
            .pAttachments = &mSwapchain.imageViews.at(i),
            .width = mSwapchain.getExtent().width,
            .height = mSwapchain.getExtent().height,
            .layers = 1
        };

        VkResult result = vkCreateFramebuffer(mRenderDevice.device,
                                              &framebufferCreateInfo,
                                              nullptr,
                                              &mFramebuffers.at(i));
        vulkanCheck(result, "Failed to create framebuffer.");

        setVulkanObjectDebugName(mRenderDevice,
                                 VK_OBJECT_TYPE_FRAMEBUFFER,
                                 std::format("VulkanImGui::mFramebuffers.at({})", i),
                                 mFramebuffers.at(i));
    }
}
