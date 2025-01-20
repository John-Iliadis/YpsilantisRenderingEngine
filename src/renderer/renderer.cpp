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
    dslBuilder.init(mRenderDevice->device);
    dsBuilder.init(mRenderDevice->device, mRenderDevice->descriptorPool);

    createDepthImages();
    createViewProjUBOs();
    createViewProjDescriptors();

    createImguiTextures();
    createImguiRenderpass();
    createImguiFramebuffers();
    initImGui(window, instance, renderDevice, mImguiRenderpass);

    createFinalRenderPass();
    createSwapchainImageFramebuffers();
    createFinalDescriptorResources();
    createFinalPipelineLayout();
    createFinalPipeline();

    mModelImporter.create(*mRenderDevice);
    mModelImporter.setImportFinishedCallback([this] (const LoadedModel& loadedModel) {addModel(loadedModel);});

    mSceneCamera = {
        glm::vec3(0.f, 0.f, -5.f),
        30.f,
        static_cast<float>(mSwapchain->extent.width),
        static_cast<float>(mSwapchain->extent.height)
    };
}

void Renderer::terminate()
{
    // meshes
    for (auto& meshPtr : mMeshes)
        meshPtr->destroy();

    // textures
    for (auto& texture : mTextures)
        destroyTexture(*mRenderDevice, texture.texture);

    // viewProj buffer and descriptor
    vkDestroyDescriptorSetLayout(mRenderDevice->device, mViewProjDSLayout, nullptr);
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        destroyBuffer(*mRenderDevice, mViewProjUBOs.at(i));

    // imgui
    terminateImGui();
    vkDestroyRenderPass(mRenderDevice->device, mImguiRenderpass, nullptr);
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        destroyTexture(*mRenderDevice, mImguiTextures.at(i));
        vkDestroyFramebuffer(mRenderDevice->device, mImguiFramebuffers.at(i), nullptr);
    }

        // final renderpass resources
    vkDestroyRenderPass(mRenderDevice->device, mFinalRenderPass, nullptr);
    vkDestroyPipelineLayout(mRenderDevice->device, mFinalPipelineLayout, nullptr);
    vkDestroyPipeline(mRenderDevice->device, mFinalPipeline, nullptr);
    vkDestroyDescriptorSetLayout(mRenderDevice->device, mFinalDSLayout, nullptr);
    for (uint32_t i = 0; i < mSwapchain->imageCount; ++i)
        vkDestroyFramebuffer(mRenderDevice->device, mSwapchainImageFramebuffers.at(i), nullptr);

    // renderer
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        destroyImage(*mRenderDevice, mDepthImages.at(i));
    }
}

void Renderer::handleEvent(const Event &event)
{
    mSceneCamera.handleEvent(event);
}

void Renderer::update(float dt)
{
    mModelImporter.processMainThreadTasks();
    updateImgui();
    mSceneCamera.update(dt);
}

void Renderer::fillCommandBuffer(VkCommandBuffer commandBuffer, uint32_t frameIndex, uint32_t imageIndex)
{
    updateUniformBuffers(frameIndex);
    renderImgui(commandBuffer, frameIndex);
    renderToSwapchainImageFinal(commandBuffer, frameIndex, imageIndex);
}

void Renderer::onSwapchainRecreate()
{
    // renderer
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        destroyImage(*mRenderDevice, mDepthImages.at(i));
    createDepthImages();

    // imgui
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        destroyTexture(*mRenderDevice, mImguiTextures.at(i));
        vkDestroyFramebuffer(mRenderDevice->device, mImguiFramebuffers.at(i), nullptr);
    }

    createImguiTextures();
    createImguiFramebuffers();

    // final renderpass
    for (uint32_t i = 0; i < mSwapchain->imageCount; ++i)
        vkDestroyFramebuffer(mRenderDevice->device, mSwapchainImageFramebuffers.at(i), nullptr);
    createSwapchainImageFramebuffers();
    updateFinalDescriptors();
}

void Renderer::addModel(const LoadedModel& loadedModel)
{
    Model model {
        .name = loadedModel.name,
        .root = loadedModel.root
    };

    // add textures to renderer
    std::unordered_map<TexturePath, uint32_t> texturePathToIndex;
    for (auto& [texturePath, namedTexture] : loadedModel.textures)
    {
        mTextures.push_back(namedTexture);
        texturePathToIndex[texturePath] = mTextures.size() - 1;
    }

    // add materials to renderer
    for (auto& material : loadedModel.materials)
    {
        Workflow workflow = Workflow::Metallic;

        if (!material.textures.at(TextureType::Specular).empty())
            workflow = Workflow::Specular;

        GpuMaterial gpuMaterial {
            .workflow = workflow,
            .albedoColor = glm::vec4(material.albedoColor, 1.f),
            .emissionColor = glm::vec4(material.emissionColor, 0.f)
        };

        if (const TexturePath& texturePath = material.textures.at(TextureType::Albedo);
            !texturePath.empty())
            gpuMaterial.albedoMapIndex = texturePathToIndex.at(texturePath);

        if (const TexturePath& texturePath = material.textures.at(TextureType::Roughness);
                !texturePath.empty())
            gpuMaterial.roughnessMapIndex = texturePathToIndex.at(texturePath);

        if (const TexturePath& texturePath = material.textures.at(TextureType::Metallic);
                !texturePath.empty())
            gpuMaterial.metallicMapIndex = texturePathToIndex.at(texturePath);

        if (const TexturePath& texturePath = material.textures.at(TextureType::Normal);
                !texturePath.empty())
            gpuMaterial.normalMapIndex = texturePathToIndex.at(texturePath);

        if (const TexturePath& texturePath = material.textures.at(TextureType::Displacement);
                !texturePath.empty())
            gpuMaterial.displacementMapIndex = texturePathToIndex.at(texturePath);

        if (const TexturePath& texturePath = material.textures.at(TextureType::Ao);
                !texturePath.empty())
            gpuMaterial.aoMapIndex = texturePathToIndex.at(texturePath);

        if (const TexturePath& texturePath = material.textures.at(TextureType::Emission);
                !texturePath.empty())
            gpuMaterial.emissionMapIndex = texturePathToIndex.at(texturePath);

        if (const TexturePath& texturePath = material.textures.at(TextureType::Specular);
                !texturePath.empty())
            gpuMaterial.specularMapIndex = texturePathToIndex.at(texturePath);

        mMaterials.push_back(gpuMaterial);

        auto namedMaterial = std::make_shared<NamedMaterial>(material.name, &mMaterials.back());
        mNamedMaterials.push_back(namedMaterial);
        model.materialsMapped.emplace(material.name, namedMaterial);
    }

    // create meshes
    for (auto& mesh : loadedModel.meshes)
    {
        mMeshes.push_back(mesh.mesh);
        model.meshes.emplace_back(mesh.mesh, loadedModel.materials.at(mesh.materialIndex).name);
    }

    mModels.push_back(std::move(model));
}

void Renderer::createDepthImages()
{
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        mDepthImages.at(i) = createImage2D(*mRenderDevice,
                                              VK_FORMAT_D32_SFLOAT,
                                              mSwapchain->extent.width,
                                              mSwapchain->extent.height,
                                              VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
                                              VK_IMAGE_ASPECT_DEPTH_BIT);

        setDebugVulkanObjectName(mRenderDevice->device,
                                 VK_OBJECT_TYPE_IMAGE,
                                 std::format("Renderer::mDepthImages[{}]", i),
                                 mDepthImages.at(i).image);
    }
}

void Renderer::createViewProjUBOs()
{
    VkBufferUsageFlags usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    VkMemoryPropertyFlags memoryProperties {
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    };

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        mViewProjUBOs.at(i) = createBuffer(*mRenderDevice, sizeof(glm::mat4), usage, memoryProperties);

        setDebugVulkanObjectName(mRenderDevice->device,
                                 VK_OBJECT_TYPE_BUFFER,
                                 std::format("Renderer::mViewProjUBOs[{}]", i),
                                 mViewProjUBOs.at(i).buffer);
    }
}

void Renderer::createViewProjDescriptors()
{
    mViewProjDSLayout = dslBuilder
        .addLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT)
        .setDebugName("mViewProjDSLayout")
        .create();

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        mViewProjDS.at(i) = dsBuilder
            .setLayout(mViewProjDSLayout)
            .addBuffer(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0,
                       mViewProjUBOs.at(i).buffer, 0, sizeof(glm::mat4))
            .setDebugName(std::format("mViewProjDS.at({})", i))
            .create();
    }
}

// todo: error handling
// todo: add caching
// todo: finish rest of logic
void Renderer::imguiMainMenuBar()
{
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("ImportModel"))
            {
                std::string modelPath = fileDialog();

                if (!modelPath.empty())
                {
                    mModelImporter.importModel(modelPath);
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
    beginImGui();
    {
        imguiMainMenuBar();
        imguiSceneGraph();
        imguiAssetPanel();
        ImGui::ShowDemoWindow();
    }
    endImGui();
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

    beginDebugLabel(commandBuffer, "Imgui Render");
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    imGuiFillCommandBuffer(commandBuffer);
    vkCmdEndRenderPass(commandBuffer);
    endDebugLabel(commandBuffer);
}

void Renderer::updateUniformBuffers(uint32_t frameIndex)
{
    mapBufferMemory(*mRenderDevice,
                    mViewProjUBOs.at(frameIndex),
                    0, sizeof(glm::mat4),
                    glm::value_ptr(mSceneCamera.viewProjection()));
}

void Renderer::createImguiTextures()
{
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        mImguiTextures.at(i) = createTexture2D(*mRenderDevice,
                                                  mSwapchain->extent.width,
                                                  mSwapchain->extent.height,
                                                  VK_FORMAT_R8G8B8A8_UNORM,
                                                  VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT |
                                                      VK_IMAGE_USAGE_SAMPLED_BIT,
                                                  VK_IMAGE_ASPECT_COLOR_BIT,
                                                  TextureWrap::ClampToEdge,
                                                  TextureFilter::Nearest);

        setImageDebugName(*mRenderDevice, mImguiTextures.at(i).image, "Renderer::mImguiTextures", i);
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
        .finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
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
                             "Renderer::mImguiRenderpass",
                             mImguiRenderpass);
}

void Renderer::createImguiFramebuffers()
{
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        VkFramebufferCreateInfo framebufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = mImguiRenderpass,
            .attachmentCount = 1,
            .pAttachments = &mImguiTextures.at(i).image.imageView,
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
                                 std::format("Renderer::mImguiFramebuffers[{}]", i),
                                 mImguiFramebuffers.at(i));
    }
}

void Renderer::createFinalRenderPass()
{
    VkAttachmentDescription colorAttachment {
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
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
        .pAttachments = &colorAttachment,
        .subpassCount = 1,
        .pSubpasses = &subpass
    };

    VkResult result = vkCreateRenderPass(mRenderDevice->device, &renderPassCreateInfo, nullptr, &mFinalRenderPass);
    vulkanCheck(result, "Failed to create present renderpass");

    setDebugVulkanObjectName(mRenderDevice->device,
                             VK_OBJECT_TYPE_RENDER_PASS,
                             "Renderer::mFinalRenderPass",
                             mFinalRenderPass);
}

void Renderer::createSwapchainImageFramebuffers()
{
    mSwapchainImageFramebuffers.resize(mSwapchain->imageCount);

    for (uint32_t i = 0; i < mSwapchain->imageCount; ++i)
    {
        VkFramebufferCreateInfo framebufferCreateInfo {
            .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
            .renderPass = mFinalRenderPass,
            .attachmentCount = 1,
            .pAttachments = &mSwapchain->images.at(i).imageView,
            .width = mSwapchain->extent.width,
            .height = mSwapchain->extent.height,
            .layers = 1
        };

        VkResult result = vkCreateFramebuffer(mRenderDevice->device,
                                              &framebufferCreateInfo,
                                              nullptr,
                                              &mSwapchainImageFramebuffers.at(i));
        vulkanCheck(result, std::format("Failed to create swapchain image framebuffer {}", i).c_str());

        setDebugVulkanObjectName(mRenderDevice->device,
                                 VK_OBJECT_TYPE_FRAMEBUFFER,
                                 std::format("Renderer::mSwapchainImageFramebuffers[{}]", i),
                                 mSwapchainImageFramebuffers.at(i));
    }
}

void Renderer::createFinalDescriptorResources()
{
    mFinalDSLayout = dslBuilder
        .addLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .setDebugName("mFinalDSLayout")
        .create();

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        mFinalDS.at(i) = dsBuilder
            .setLayout(mFinalDSLayout)
            .addTexture(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, mImguiTextures.at(i))
            .setDebugName(std::format("mFinalDS.at({})", i))
            .create();
    }
}

void Renderer::createFinalPipelineLayout()
{
    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &mFinalDSLayout
    };

    VkResult result = vkCreatePipelineLayout(mRenderDevice->device,
                                             &pipelineLayoutCreateInfo,
                                             nullptr,
                                             &mFinalPipelineLayout);
    vulkanCheck(result, "Failed to create present pipeline layout");

    setDebugVulkanObjectName(mRenderDevice->device,
                             VK_OBJECT_TYPE_PIPELINE_LAYOUT,
                             "Renderer::mFinalPipelineLayout",
                             mFinalPipelineLayout);
}

void Renderer::createFinalPipeline()
{
    VulkanShaderModule vertexShaderModule(mRenderDevice->device, "fullScreenQuad.vert");
    VulkanShaderModule fragmentShaderModule(mRenderDevice->device, "imageToQuad.frag");

    std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages {
        shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShaderModule),
        shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShaderModule)
    };

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST
    };

    VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO
    };

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .cullMode = VK_CULL_MODE_NONE,
        .lineWidth = 1.f
    };

    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT
    };

    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState {
        .colorWriteMask {
            VK_COLOR_COMPONENT_R_BIT |
            VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT |
            VK_COLOR_COMPONENT_A_BIT
        }
    };

    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachmentState
    };

    std::array<VkDynamicState, 2> dynamicStates {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(dynamicStates.size()),
        .pDynamicStates = dynamicStates.data()
    };

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = static_cast<uint32_t>(shaderStages.size()),
        .pStages = shaderStages.data(),
        .pVertexInputState = &vertexInputStateCreateInfo,
        .pInputAssemblyState = &inputAssemblyStateCreateInfo,
        .pTessellationState = &tessellationStateCreateInfo,
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &rasterizationStateCreateInfo,
        .pMultisampleState = &multisampleStateCreateInfo,
        .pDepthStencilState = &depthStencilStateCreateInfo,
        .pColorBlendState = &colorBlendStateCreateInfo,
        .pDynamicState = &dynamicStateCreateInfo,
        .layout = mFinalPipelineLayout,
        .renderPass = mFinalRenderPass,
        .subpass = 0
    };

    VkResult result = vkCreateGraphicsPipelines(mRenderDevice->device,
                                                VK_NULL_HANDLE,
                                                1, &graphicsPipelineCreateInfo,
                                                nullptr,
                                                &mFinalPipeline);
    vulkanCheck(result, "Failed to create present pipeline.");

    setDebugVulkanObjectName(mRenderDevice->device,
                             VK_OBJECT_TYPE_PIPELINE,
                             "Renderer::mPresentPipeline",
                             mFinalPipeline);
}

void Renderer::renderToSwapchainImageFinal(VkCommandBuffer commandBuffer, uint32_t frameIndex, uint32_t imageIndex)
{
    VkRenderPassBeginInfo renderPassBeginInfo {
        .sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
        .renderPass = mFinalRenderPass,
        .framebuffer = mSwapchainImageFramebuffers.at(imageIndex),
        .renderArea {
            .offset {.x = 0, .y = 0},
            .extent {
                .width = mSwapchain->extent.width,
                .height = mSwapchain->extent.height
            },
        },
        .clearValueCount = 0
    };

    VkViewport viewport {
        .x = 0,
        .y = 0,
        .width = static_cast<float>(mSwapchain->extent.width),
        .height = static_cast<float>(mSwapchain->extent.height),
        .minDepth = 0.f,
        .maxDepth = 1.f
    };

    VkRect2D scissor {
        .offset = {0, 0},
        .extent = mSwapchain->extent
    };

    beginDebugLabel(commandBuffer, "Final Renderpass");
    vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, mFinalPipeline);
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
    vkCmdBindDescriptorSets(commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            mFinalPipelineLayout,
                            0, 1, &mFinalDS.at(frameIndex),
                            0, nullptr);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    vkCmdEndRenderPass(commandBuffer);
    endDebugLabel(commandBuffer);
}

void Renderer::updateFinalDescriptors()
{
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        VkDescriptorImageInfo imageInfo {
            .sampler = mImguiTextures.at(i).sampler,
            .imageView = mImguiTextures.at(i).image.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        VkWriteDescriptorSet writeDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = mFinalDS.at(i),
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &imageInfo
        };

        vkUpdateDescriptorSets(mRenderDevice->device, 1, &writeDescriptorSet, 0, nullptr);
    }
}
