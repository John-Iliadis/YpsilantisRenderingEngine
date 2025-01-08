//
// Created by Gianni on 4/01/2025.
//

#include "renderer.hpp"

void Renderer::init(GLFWwindow* window,
                    const VulkanInstance& instance,
                    const VulkanRenderDevice& renderDevice,
                    const VulkanSwapchain& swapchain)
{
    mRenderDevice = &renderDevice;
    mSwapchain = &swapchain;

    createDescriptorPool();
    createDepthImages();
    createViewProjUBOs();
    createViewProjDescriptors();
    createImguiRenderpass();
    createImguiFramebuffers();

    initImGui(window, instance, renderDevice, mDescriptorPool, mImguiRenderpass);

    mSceneCamera = {
        glm::vec3(0.f, 0.f, -5.f),
        30.f,
        static_cast<float>(mSwapchain->extent.width),
        static_cast<float>(mSwapchain->extent.height)
    };
}

void Renderer::terminate()
{
    terminateImGui();

    for (uint32_t i = 0; i < VulkanSwapchain::swapchainImageCount(); ++i)
    {
        destroyImage(*mRenderDevice, mDepthImages.at(i));
        destroyBuffer(*mRenderDevice, mViewProjUBOs.at(i));
        vkDestroyFramebuffer(mRenderDevice->device, mImguiFramebuffers.at(i), nullptr);
    }

    vkDestroyRenderPass(mRenderDevice->device, mImguiRenderpass, nullptr);
    vkDestroyDescriptorPool(mRenderDevice->device, mDescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(mRenderDevice->device, mViewProjDSLayout, nullptr);
}

void Renderer::handleEvent(const Event &event)
{
    mSceneCamera.handleEvent(event);
}

void Renderer::update(float dt)
{
    beginImGui();
    mSceneCamera.update(dt);
    ImGui::ShowDemoWindow();
}

void Renderer::fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    updateUniformBuffers(imageIndex);
    renderImgui(commandBuffer, imageIndex);
}

void Renderer::createDescriptorPool()
{
    mDescriptorPool = ::createDescriptorPool(*mRenderDevice, 1000, 100, 100, 100);

    setDebugVulkanObjectName(mRenderDevice->device,
                             VK_OBJECT_TYPE_DESCRIPTOR_POOL,
                             "Renderer descriptor pool",
                             mDescriptorPool);
}

void Renderer::createDepthImages()
{
    mDepthImages.resize(VulkanSwapchain::swapchainImageCount());

    for (uint32_t i = 0; i < mDepthImages.size(); ++i)
    {
        mDepthImages.at(i) = createImage2D(*mRenderDevice,
                                              VK_FORMAT_D32_SFLOAT,
                                              mSwapchain->extent.width,
                                              mSwapchain->extent.height,
                                              VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                              VK_IMAGE_ASPECT_DEPTH_BIT);

        setDebugVulkanObjectName(mRenderDevice->device,
                                 VK_OBJECT_TYPE_IMAGE,
                                 std::format("Application depth image {}", i),
                                 mDepthImages.at(i).image);
    }
}

void Renderer::createViewProjUBOs()
{
    mViewProjUBOs.resize(VulkanSwapchain::swapchainImageCount());

    VkBufferUsageFlags usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    VkMemoryPropertyFlags memoryProperties {
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    };

    for (size_t i = 0; i < mViewProjUBOs.size(); ++i)
    {
        mViewProjUBOs.at(i) = createBuffer(*mRenderDevice, sizeof(glm::mat4), usage, memoryProperties);

        setDebugVulkanObjectName(mRenderDevice->device,
                                 VK_OBJECT_TYPE_BUFFER,
                                 std::format("ViewProjection buffer {}", i),
                                 mViewProjUBOs.at(i).buffer);
    }
}

void Renderer::createViewProjDescriptors()
{
    mViewProjDS.resize(VulkanSwapchain::swapchainImageCount());

    DescriptorSetLayoutCreator DSLayoutCreator(mRenderDevice->device);
    DSLayoutCreator.addLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT);
    mViewProjDSLayout = DSLayoutCreator.create();

    setDebugVulkanObjectName(mRenderDevice->device,
                             VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                             "ViewProjection descriptor set layout",
                             mViewProjDSLayout);

    for (uint32_t i = 0; i < mViewProjDS.size(); ++i)
    {
        DescriptorSetCreator descriptorSetCreator(mRenderDevice->device, mDescriptorPool, mViewProjDSLayout);
        descriptorSetCreator.addBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                                       0, mViewProjUBOs.at(i).buffer,
                                       0, sizeof(glm::mat4));
        mViewProjDS.at(i) = descriptorSetCreator.create();

        setDebugVulkanObjectName(mRenderDevice->device,
                                 VK_OBJECT_TYPE_DESCRIPTOR_SET,
                                 std::format("ViewProjection descriptor set {}", i),
                                 mViewProjDS.at(i));
    }
}

void Renderer::renderImgui(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    VkClearValue clearValue {.color = {0.2f, 0.2f, 0.2f, 1.f}};

    VkRenderPassBeginInfo renderPassBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mImguiRenderpass,
        .framebuffer = mImguiFramebuffers.at(imageIndex),
        .renderArea = {
            .offset = {.x = 0, .y = 0},
            .extent = mSwapchain->extent
        },
        .clearValueCount = 1,
        .pClearValues = &clearValue
    };

    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    ::renderImGui(commandBuffer);
    vkCmdEndRenderPass(commandBuffer);
}

void Renderer::updateUniformBuffers(uint32_t imageIndex)
{
    mapBufferMemory(*mRenderDevice,
                    mViewProjUBOs.at(imageIndex),
                    0, sizeof(glm::mat4),
                    glm::value_ptr(mSceneCamera.viewProjection()));
}

void Renderer::createImguiRenderpass()
{
    VkAttachmentDescription attachmentDescription {
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR, // todo: change to VK_ATTACHMENT_LOAD_OP_LOAD once post processing is implemented
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED, // todo: change to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL once post processing is implemented
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    };

    VkAttachmentReference colorAttachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
        .inputAttachmentCount = 0,
        .colorAttachmentCount = 1,
        .pColorAttachments = &colorAttachmentRef
    };

    VkRenderPassCreateInfo renderPassCreateInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &attachmentDescription,
        .subpassCount = 1,
        .pSubpasses = &subpass
    };

    VkResult result = vkCreateRenderPass(mRenderDevice->device, &renderPassCreateInfo, nullptr, &mImguiRenderpass);
    vulkanCheck(result, "Failed to create imgui renderpass.");

    setDebugVulkanObjectName(mRenderDevice->device,
                             VK_OBJECT_TYPE_RENDER_PASS,
                             "Imgui renderpass",
                             mImguiRenderpass);
}

void Renderer::createImguiFramebuffers()
{
    mImguiFramebuffers.resize(VulkanSwapchain::swapchainImageCount());

    for (size_t i = 0; i < mImguiFramebuffers.size(); ++i)
    {
        VkFramebufferCreateInfo framebufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = mImguiRenderpass,
            .attachmentCount = 1,
            .pAttachments = &mSwapchain->images.at(i).imageView,
            .width = mSwapchain->extent.width,
            .height = mSwapchain->extent.height,
            .layers = 1
        };

        VkResult result = vkCreateFramebuffer(mRenderDevice->device,
                                              &framebufferCreateInfo,
                                              nullptr,
                                              &mImguiFramebuffers.at(i));
        vulkanCheck(result, std::format("Failed to create imgui framebuffer {}", i).c_str());

        setDebugVulkanObjectName(mRenderDevice->device,
                                 VK_OBJECT_TYPE_FRAMEBUFFER,
                                 std::format("Imgui framebuffer {}", i),
                                 mImguiFramebuffers.at(i));
    }
}

void Renderer::onSwapchainRecreate()
{
    for (uint32_t i = 0; i < VulkanSwapchain::swapchainImageCount(); ++i)
    {
        destroyImage(*mRenderDevice, mDepthImages.at(i));
        vkDestroyFramebuffer(mRenderDevice->device, mImguiFramebuffers.at(i), nullptr);
    }

    createDepthImages();
    createImguiFramebuffers();
}
