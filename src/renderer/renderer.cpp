//
// Created by Gianni on 4/01/2025.
//

#include "renderer.hpp"

Renderer::Renderer(VulkanRenderDevice *renderDevice, VulkanSwapchain *swapchain)
    : mRenderDevice(renderDevice)
    , mSwapchain(swapchain)
{
}

void Renderer::init()
{
    createDescriptorPool();
    createDepthImages();
    createViewProjUBOs();
    createViewProjDescriptors();
}

void Renderer::terminate()
{
    for (uint32_t i = 0; i < VulkanSwapchain::swapchainImageCount(); ++i)
    {
        destroyImage(*mRenderDevice, mDepthImages.at(i));
        destroyBuffer(*mRenderDevice, mViewProjUBOs.at(i));
    }

    vkDestroyDescriptorPool(mRenderDevice->device, mDescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(mRenderDevice->device, mViewProjDSLayout, nullptr);
}

void Renderer::handleEvent(const Event &event)
{
    mSceneCamera.handleEvent(event);
}

void Renderer::update(float dt)
{
    mSceneCamera.update(dt);
}

void Renderer::fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
{
    updateUniformBuffers(imageIndex);
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

void Renderer::updateUniformBuffers(uint32_t imageIndex)
{
    mapBufferMemory(*mRenderDevice,
                    mViewProjUBOs.at(imageIndex),
                    0, sizeof(glm::mat4),
                    glm::value_ptr(mSceneCamera.viewProjection()));
}

void Renderer::onSwapchainRecreate()
{
    for (uint32_t i = 0; i < VulkanSwapchain::swapchainImageCount(); ++i)
    {
        destroyImage(*mRenderDevice, mDepthImages.at(i));
    }
}
