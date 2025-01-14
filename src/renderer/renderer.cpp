//
// Created by Gianni on 4/01/2025.
//

#include "renderer.hpp"

void Renderer::init(GLFWwindow* window,
                    VulkanInstance& instance,
                    VulkanRenderDevice& renderDevice,
                    VulkanSwapchain& swapchain)
{
    mRenderDevice = &renderDevice;
    mSwapchain = &swapchain;
    mMsaaSampleCount = VkSampleCountFlagBits(glm::min(uint32_t(VK_SAMPLE_COUNT_8_BIT), uint32_t(getMaxSampleCount(mRenderDevice->properties))));

    createDescriptorPool();
    createDepthImages();
    createViewProjUBOs();
    createViewProjDescriptors();

    createImguiImages();
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
    for (auto& [meshID, mesh] : mMeshes)
        mesh.destroy(*mRenderDevice);

    terminateImGui();

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        destroyImage(*mRenderDevice, mDepthImages.at(i));
        destroyBuffer(*mRenderDevice, mViewProjUBOs.at(i));

        destroyImage(*mRenderDevice, mImguiImages.at(i));
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
    uploadLoadedResources();
    beginImGui();
    updateImgui();
    mSceneCamera.update(dt);
}

void Renderer::fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t frameIndex, uint32_t swapchainImageIndex)
{
    updateUniformBuffers(frameIndex);
    renderImgui(commandBuffer, frameIndex);
    blitToSwapchainImage(commandBuffer, frameIndex, swapchainImageIndex);
}

void Renderer::onSwapchainRecreate()
{
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        destroyImage(*mRenderDevice, mDepthImages.at(i));
        destroyImage(*mRenderDevice, mImguiImages.at(i));
        vkDestroyFramebuffer(mRenderDevice->device, mImguiFramebuffers.at(i), nullptr);
    }

    createDepthImages();
    createImguiImages();
    createImguiFramebuffers();
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
    mDepthImages.resize(MAX_FRAMES_IN_FLIGHT);

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
    mViewProjUBOs.resize(MAX_FRAMES_IN_FLIGHT);

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
    mViewProjDS.resize(MAX_FRAMES_IN_FLIGHT);

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

// todo: finish rest of logic
void Renderer::imguiMainMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Import"))
            {
                std::string modelPath = fileDialog();

                if (!modelPath.empty() && !mLoadedModels.contains(modelPath))
                {
                    importModel(modelPath);
                    mLoadedModels.insert(modelPath);
                }
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Settings"))
        {
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View"))
        {
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }
}

void Renderer::imguiAssetPanel()
{
    ImGui::Begin("Assets");

    for (const auto& [modelID, model] : mModels)
        ImGui::Button(model.name.c_str());

    ImGui::End();
}

// todo: implement selected node
void Renderer::imguiSceneNodeRecursive(SceneNode *sceneNode)
{
    static constexpr ImGuiTreeNodeFlags treeNodeFlags {
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_OpenOnDoubleClick |
        ImGuiTreeNodeFlags_SpanAvailWidth
    };

    if (ImGui::TreeNodeEx((void*)(intptr_t)sceneNode, treeNodeFlags, sceneNode->name().c_str()))
    {
        imguiSceneNodeDragDropSource(sceneNode);
        imguiSceneNodeDragDropTarget(sceneNode);

        for (SceneNode* child : sceneNode->children())
            imguiSceneNodeRecursive(child);

        ImGui::TreePop();
    }
}

void Renderer::imguiSceneNodeDragDropSource(SceneNode *sceneNode)
{
    if (ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload("SceneNode", &sceneNode, sizeof(SceneNode*));
        ImGui::EndDragDropSource();
    }
}

void Renderer::imguiSceneNodeDragDropTarget(SceneNode *sceneNode)
{
    if (ImGui::BeginDragDropTarget())
    {
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("SceneNode");

        if (payload)
        {
            SceneNode* transferNode = *(SceneNode**)payload->Data;

            transferNode->orphan();
            sceneNode->addChild(transferNode);
        }

        ImGui::EndDragDropTarget();
    }
}

void Renderer::imguiSceneGraph()
{
    ImGui::Begin("Scene");

    for (SceneNode* child : mSceneRoot.children())
        imguiSceneNodeRecursive(child);

    ImGui::Dummy(ImGui::GetContentRegionAvail());
    imguiSceneNodeDragDropTarget(&mSceneRoot);

    ImGui::End();
}

void Renderer::updateImgui()
{
    imguiMainMenuBar();
    imguiSceneGraph();
    imguiAssetPanel();
    ImGui::ShowDemoWindow();
}

void Renderer::renderImgui(VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
    VkClearValue clearValue {{0.2f, 0.2f, 0.2f, 1.f}};

    VkRenderPassBeginInfo renderPassBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mImguiRenderpass,
        .framebuffer = mImguiFramebuffers.at(frameIndex),
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

void Renderer::updateUniformBuffers(uint32_t frameIndex)
{
    mapBufferMemory(*mRenderDevice,
                    mViewProjUBOs.at(frameIndex),
                    0, sizeof(glm::mat4),
                    glm::value_ptr(mSceneCamera.viewProjection()));
}

void Renderer::createImguiImages()
{
    mImguiImages.resize(MAX_FRAMES_IN_FLIGHT);

    for (uint32_t i = 0; i < mImguiImages.size(); ++i)
    {
        mImguiImages.at(i) = createImage2D(*mRenderDevice,
                                           VK_FORMAT_R8G8B8A8_UNORM,
                                           mSwapchain->extent.width,
                                           mSwapchain->extent.height,
                                           VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                               VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
                                           VK_IMAGE_ASPECT_COLOR_BIT);

        setImageDebugName(*mRenderDevice, mImguiImages.at(i), "Imgui", i);
    }
}

void Renderer::createImguiRenderpass()
{
    VkAttachmentDescription attachmentDescription {
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL
    };

    VkAttachmentReference colorAttachmentRef {
        .attachment = 0,
        .layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    };

    VkSubpassDescription subpass {
        .pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
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
    mImguiFramebuffers.resize(MAX_FRAMES_IN_FLIGHT);

    for (size_t i = 0; i < mImguiFramebuffers.size(); ++i)
    {
        VkFramebufferCreateInfo framebufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = mImguiRenderpass,
            .attachmentCount = 1,
            .pAttachments = &mImguiImages.at(i).imageView,
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

// todo: create a renderpass instead of blit
void Renderer::blitToSwapchainImage(VkCommandBuffer commandBuffer, uint32_t frameIndex, uint32_t swapchainImageIndex)
{
    transitionImageLayout(commandBuffer,
                          mSwapchain->images.at(swapchainImageIndex),
                          VK_IMAGE_LAYOUT_UNDEFINED,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    int32_t width = mSwapchain->extent.width;
    int32_t height = mSwapchain->extent.height;

    VkImageBlit blitRegion {
        .srcSubresource {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .srcOffsets {
            {.x = 0, .y = 0, .z = 0},
            {.x = width, .y = height, .z = 1}
        },
        .dstSubresource {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1
        },
        .dstOffsets {
            {.x = 0, .y = 0, .z = 0},
            {.x = width, .y = height, .z = 1}
        }
    };

    vkCmdBlitImage(commandBuffer,
                   mImguiImages.at(frameIndex).image,
                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                   mSwapchain->images.at(swapchainImageIndex).image,
                   VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                   1, &blitRegion,
                   VK_FILTER_NEAREST);

    transitionImageLayout(commandBuffer,
                          mSwapchain->images.at(swapchainImageIndex),
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                          VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
}

void Renderer::uploadLoadedResources()
{
    if (!mLoadedMeshData.empty())
    {
        std::lock_guard<std::mutex> lock(mLoadedMeshDataMutex);

        for (const auto& loadedMeshData : mLoadedMeshData)
        {
            Model& model = mModels.at(loadedMeshData.modelId);

            Mesh mesh;
            mesh.create(*mRenderDevice,
                        loadedMeshData.vertices.size(),
                        loadedMeshData.vertices.data(),
                        loadedMeshData.indices.size(),
                        loadedMeshData.indices.data(),
                        loadedMeshData.meshName);

            model.meshes.push_back(mesh.id());
            model.meshNodeMap.try_emplace(mesh.id(), 0, loadedMeshData.transformation);

            mMeshes.try_emplace(mesh.id(), std::move(mesh));
        }

        mLoadedMeshData.clear();
    }
}

// todo: fix meshes with no name
void Renderer::importModel(const std::string &filename)
{
    std::string modelName = filename.substr(filename.find_last_of('/') + 1);
    uint32_t modelId = std::hash<std::string>()(modelName);

    mModels.try_emplace(modelId, modelId, std::move(modelName));

    auto future = std::async([this, filename, modelId] () {
        std::unique_ptr<aiScene> scene = loadScene(filename);

        std::deque<aiNode*> nodes(1, scene->mRootNode);

        while (!nodes.empty())
        {
            const aiNode& node = *nodes.back();
            nodes.pop_back();

            for (uint32_t i = 0; i < node.mNumMeshes; ++i)
            {
                const aiMesh& mesh = *scene->mMeshes[node.mMeshes[i]];

                LoadedMeshData loadedMeshData {
                    .modelId = modelId,
                    .meshName = mesh.mName.C_Str(),
                    .vertices = loadMeshVertices(mesh),
                    .indices = loadMeshIndices(mesh),
                    .transformation = assimpToGlmMat4(node.mTransformation)
                };

                mLoadedMeshDataMutex.lock();
                mLoadedMeshData.push_back(std::move(loadedMeshData));
                mLoadedMeshDataMutex.unlock();
            }

            for (uint32_t i = 0; i < node.mNumChildren; ++i)
                nodes.push_back(node.mChildren[i]);
        }
    });

    mImportFutures.push_back(std::move(future));
}
