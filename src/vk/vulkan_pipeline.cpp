//
// Created by Gianni on 5/01/2025.
//

#include "vulkan_pipeline.hpp"

// -- VulkanGraphicsPipeline -- //

VulkanGraphicsPipeline::VulkanGraphicsPipeline()
    : mRenderDevice()
    , mPipelineLayout()
    , mPipeline()
{
}

VulkanGraphicsPipeline::VulkanGraphicsPipeline(const VulkanRenderDevice &renderDevice, const PipelineSpecification &specification)
    : mRenderDevice(&renderDevice)
{
    createPipelineLayout(specification);
    createPipeline(specification);
}

VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
{
    if (mRenderDevice)
    {
        vkDestroyPipeline(mRenderDevice->device, mPipeline, nullptr);
        vkDestroyPipelineLayout(mRenderDevice->device, mPipelineLayout, nullptr);
        mRenderDevice = nullptr;
    }
}

VulkanGraphicsPipeline::VulkanGraphicsPipeline(VulkanGraphicsPipeline &&other) noexcept
    : VulkanGraphicsPipeline()
{
    swap(other);
}

VulkanGraphicsPipeline &VulkanGraphicsPipeline::operator=(VulkanGraphicsPipeline &&other) noexcept
{
    if (this != &other)
        swap(other);
    return *this;
}

void VulkanGraphicsPipeline::swap(VulkanGraphicsPipeline &other) noexcept
{
    std::swap(mRenderDevice, other.mRenderDevice);
    std::swap(mPipelineLayout, other.mPipelineLayout);
    std::swap(mPipeline, other.mPipeline);
}

VulkanGraphicsPipeline::operator VkPipeline() const
{
    return mPipeline;
}

VulkanGraphicsPipeline::operator VkPipelineLayout() const
{
    return mPipelineLayout;
}

void VulkanGraphicsPipeline::createPipelineLayout(const PipelineSpecification &specification)
{
    const auto& dsLayouts = specification.pipelineLayout.dsLayouts;
    const auto& pushConstantRange = specification.pipelineLayout.pushConstantRange;

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = static_cast<uint32_t>(dsLayouts.size()),
        .pSetLayouts = dsLayouts.data(),
    };

    if (pushConstantRange.has_value())
    {
        pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
        pipelineLayoutCreateInfo.pPushConstantRanges = &*pushConstantRange;
    }

    VkResult result = vkCreatePipelineLayout(mRenderDevice->device,
                                             &pipelineLayoutCreateInfo,
                                             nullptr,
                                             &mPipelineLayout);
    vulkanCheck(result, "Failed to create pipeline layout.");

    if (const auto& debugName = specification.debugName)
    {
        setVulkanObjectDebugName(*mRenderDevice,
                                 VK_OBJECT_TYPE_PIPELINE_LAYOUT,
                                 *debugName,
                                 mPipelineLayout);
    }
}

void VulkanGraphicsPipeline::createPipeline(const PipelineSpecification &specification)
{
    std::vector<VkPipelineShaderStageCreateInfo> shaderStageInfos;

    VulkanShaderModule vertShaderModule(*mRenderDevice, specification.shaderStages.vertShaderPath);
    VulkanShaderModule fragShaderModule(*mRenderDevice, specification.shaderStages.fragShaderPath);

    shaderStageInfos.push_back(shaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertShaderModule));
    shaderStageInfos.push_back(shaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragShaderModule));

    VulkanShaderModule tcsShaderModule;
    VulkanShaderModule tesShaderModule;
    VulkanShaderModule geomShaderModule;

    if (specification.shaderStages.tcsShaderPath.has_value())
    {
        assert(specification.shaderStages.tesShaderPath.has_value());

        const auto& tcsPath = specification.shaderStages.tcsShaderPath.value();
        const auto& tesPath = specification.shaderStages.tesShaderPath.value();

        tcsShaderModule = VulkanShaderModule(*mRenderDevice, tcsPath);
        tesShaderModule = VulkanShaderModule(*mRenderDevice, tesPath);

        shaderStageInfos.push_back(shaderStageCreateInfo(VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT, tcsShaderModule));
        shaderStageInfos.push_back(shaderStageCreateInfo(VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT, tesShaderModule));
    }

    if (const auto& geomPath = specification.shaderStages.geomShaderPath)
    {
        geomShaderModule = VulkanShaderModule(*mRenderDevice, *geomPath);
        shaderStageInfos.push_back(shaderStageCreateInfo(VK_SHADER_STAGE_GEOMETRY_BIT, geomShaderModule));
    }

    VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = static_cast<uint32_t>(specification.vertexInput.bindings.size()),
        .pVertexBindingDescriptions = specification.vertexInput.bindings.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(specification.vertexInput.attributes.size()),
        .pVertexAttributeDescriptions = specification.vertexInput.attributes.data(),
    };

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = specification.inputAssembly.topology,
    };

    VkPipelineTessellationStateCreateInfo tessellationStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        .patchControlPoints = specification.tesselation.patchControlUnits
    };

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1
    };

    VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .rasterizerDiscardEnable = specification.rasterization.rasterizerDiscardPrimitives,
        .polygonMode = specification.rasterization.polygonMode,
        .cullMode = specification.rasterization.cullMode,
        .frontFace = specification.rasterization.frontFace,
        .lineWidth = specification.rasterization.lineWidth
    };

    VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = specification.multisampling.samples
    };

    VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = specification.depthStencil.enableDepthTest,
        .depthWriteEnable = specification.depthStencil.enableDepthWrite,
        .depthCompareOp = specification.depthStencil.depthCompareOp,
        .stencilTestEnable = VK_FALSE
    };

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState {
        .blendEnable = specification.blending.enable,
        .srcColorBlendFactor = specification.blending.srcColorBlendFactor,
        .dstColorBlendFactor = specification.blending.dstColorBlendFactor,
        .colorBlendOp = specification.blending.colorBlendOp,
        .srcAlphaBlendFactor = specification.blending.srcAlphaBlendFactor,
        .dstAlphaBlendFactor = specification.blending.dstAlphaBlendFactor,
        .alphaBlendOp = specification.blending.alphaBlendOp,
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT |
                          VK_COLOR_COMPONENT_G_BIT |
                          VK_COLOR_COMPONENT_B_BIT |
                          VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .attachmentCount = 1,
        .pAttachments = &colorBlendAttachmentState
    };

    VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = static_cast<uint32_t>(specification.dynamicStates.size()),
        .pDynamicStates = specification.dynamicStates.data()
    };

    VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = static_cast<uint32_t>(shaderStageInfos.size()),
        .pStages = shaderStageInfos.data(),
        .pVertexInputState = &vertexInputStateCreateInfo,
        .pInputAssemblyState = &inputAssemblyStateCreateInfo,
        .pTessellationState = &tessellationStateCreateInfo,
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &rasterizationStateCreateInfo,
        .pMultisampleState = &multisampleStateCreateInfo,
        .pDepthStencilState = &depthStencilStateCreateInfo,
        .pColorBlendState = &colorBlendStateCreateInfo,
        .pDynamicState = &dynamicStateCreateInfo,
        .layout = mPipelineLayout,
        .renderPass = specification.renderPass,
        .subpass = specification.subpassIndex
    };

    VkResult result = vkCreateGraphicsPipelines(mRenderDevice->device,
                                                VK_NULL_HANDLE,
                                                1, &graphicsPipelineCreateInfo,
                                                nullptr,
                                                &mPipeline);
    vulkanCheck(result, "Failed to create pipeline.");

    if (const auto& debugName = specification.debugName)
    {
        setVulkanObjectDebugName(*mRenderDevice,
                                 VK_OBJECT_TYPE_PIPELINE,
                                 *debugName,
                                 mPipeline);
    }
}

// -- VulkanShaderModule -- //

VulkanShaderModule::VulkanShaderModule()
    : mRenderDevice()
    , mShaderModule()
{
}

VulkanShaderModule::VulkanShaderModule(const VulkanRenderDevice& renderDevice, const std::string& path)
    : mRenderDevice(&renderDevice)
    , mShaderModule()
{
    std::ifstream shaderFile(path, std::ios::binary | std::ios::ate);

    check(shaderFile.is_open(), std::format("Failed to open {}", path).c_str());

    std::streamsize bufferSize = shaderFile.tellg();
    std::vector<uint32_t> shaderSrc(bufferSize / 4);

    shaderFile.seekg(0);
    shaderFile.read(reinterpret_cast<char*>(shaderSrc.data()), bufferSize);

    VkShaderModuleCreateInfo shaderModuleCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = 4 * shaderSrc.size(),
        .pCode = shaderSrc.data()
    };

    VkResult result = vkCreateShaderModule(mRenderDevice->device, &shaderModuleCreateInfo, nullptr, &mShaderModule);
    vulkanCheck(result, "Failed to create shader module.");

    setVulkanObjectDebugName(*mRenderDevice,
                             VK_OBJECT_TYPE_SHADER_MODULE,
                             path,
                             mShaderModule);
}

VulkanShaderModule::~VulkanShaderModule()
{
    if (mRenderDevice)
    {
        vkDestroyShaderModule(mRenderDevice->device, mShaderModule, nullptr);
        mRenderDevice = nullptr;
    }
}

VulkanShaderModule::operator VkShaderModule() const
{
    return mShaderModule;
}

VulkanShaderModule::VulkanShaderModule(VulkanShaderModule &&other) noexcept
    : VulkanShaderModule()
{
    swap(other);
}

VulkanShaderModule &VulkanShaderModule::operator=(VulkanShaderModule &&other) noexcept
{
    if (this != &other)
        swap(other);
    return *this;
}

void VulkanShaderModule::swap(VulkanShaderModule &other) noexcept
{
    std::swap(mRenderDevice, other.mRenderDevice);
    std::swap(mShaderModule, other.mShaderModule);
}

// -- other -- //

VkPipelineShaderStageCreateInfo shaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule module)
{
    return {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = stage,
        .module = module,
        .pName = "main"
    };
}
