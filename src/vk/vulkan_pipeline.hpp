//
// Created by Gianni on 5/01/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_PIPELINE_HPP
#define VULKANRENDERINGENGINE_VULKAN_PIPELINE_HPP

#include "vulkan_render_device.hpp"
#include "vulkan_utils.hpp"
#include "../utils/utils.hpp"

struct PipelineSpecification
{
    struct ShaderStages {
        std::string vertShaderPath;
        std::optional<std::string> tcsShaderPath;
        std::optional<std::string> tesShaderPath;
        std::optional<std::string> geomShaderPath;
        std::string fragShaderPath;
    } shaderStages;

    struct VertexInput {
        std::vector<VkVertexInputBindingDescription> bindings;
        std::vector<VkVertexInputAttributeDescription> attributes;
    } vertexInput;

    struct InputAssembly {
        VkPrimitiveTopology topology;
    } inputAssembly;

    struct TesselationState {
        uint32_t patchControlUnits;
    } tesselation;

    struct ViewportState {
        std::optional<VkViewport> viewport;
        std::optional<VkRect2D> scissor;
    } viewportState;

    struct Rasterization {
        bool rasterizerDiscardPrimitives;
        VkPolygonMode polygonMode;
        VkCullModeFlags cullMode;
        VkFrontFace frontFace;
        float lineWidth;
    } rasterization;

    struct Multisampling {
        VkSampleCountFlagBits samples;
    } multisampling;

    struct DepthStencilState {
        bool enableDepthTest;
        bool enableDepthWrite;
        VkCompareOp depthCompareOp;
    } depthStencil;

    struct BlendState {
        bool enable;
        VkBlendFactor srcColorBlendFactor;
        VkBlendFactor dstColorBlendFactor;
        VkBlendOp colorBlendOp;
        VkBlendFactor srcAlphaBlendFactor;
        VkBlendFactor dstAlphaBlendFactor;
        VkBlendOp alphaBlendOp;
    } blending;

    std::vector<VkDynamicState> dynamicStates;

    struct PipelineLayout {
        std::vector<VkDescriptorSetLayout> dsLayouts;
        std::optional<VkPushConstantRange> pushConstantRange;
    } pipelineLayout;

    VkRenderPass renderPass;
    uint32_t subpassIndex;

    std::optional<std::string> debugName;
};

class VulkanGraphicsPipeline
{
public:
    VulkanGraphicsPipeline();
    VulkanGraphicsPipeline(const VulkanRenderDevice& renderDevice, const PipelineSpecification& specification);
    ~VulkanGraphicsPipeline();

    VulkanGraphicsPipeline(const VulkanGraphicsPipeline&) = delete;
    VulkanGraphicsPipeline& operator=(const VulkanGraphicsPipeline&) = delete;

    VulkanGraphicsPipeline(VulkanGraphicsPipeline&& other) noexcept;
    VulkanGraphicsPipeline& operator=(VulkanGraphicsPipeline&& other) noexcept;

    void swap(VulkanGraphicsPipeline& other) noexcept;

    operator VkPipeline() const;
    operator VkPipelineLayout() const;

private:
    void createPipelineLayout(const PipelineSpecification& specification);
    void createPipeline(const PipelineSpecification& specification);

private:
    const VulkanRenderDevice* mRenderDevice;
    VkPipelineLayout mPipelineLayout;
    VkPipeline mPipeline;
};

class VulkanShaderModule
{
public:
    VulkanShaderModule();
    VulkanShaderModule(const VulkanRenderDevice& renderDevice, const std::string& path);
    ~VulkanShaderModule();

    VulkanShaderModule(const VulkanShaderModule&) = delete;
    VulkanShaderModule& operator=(const VulkanShaderModule&) = delete;

    VulkanShaderModule(VulkanShaderModule&& other) noexcept;
    VulkanShaderModule& operator=(VulkanShaderModule&& other) noexcept;

    void swap(VulkanShaderModule& other) noexcept;

    operator VkShaderModule() const;

private:
    const VulkanRenderDevice* mRenderDevice;
    VkShaderModule mShaderModule;
};

VkPipelineShaderStageCreateInfo shaderStageCreateInfo(VkShaderStageFlagBits stage, VkShaderModule module);

#endif //VULKANRENDERINGENGINE_VULKAN_PIPELINE_HPP
