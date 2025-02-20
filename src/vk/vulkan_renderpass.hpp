//
// Created by Gianni on 18/02/2025.
//

#ifndef VULKANRENDERINGENGINE_VULKAN_RENDERPASS_HPP
#define VULKANRENDERINGENGINE_VULKAN_RENDERPASS_HPP

struct SubpassSpecification
{
    VkPipelineBindPoint pipelineBindPoint;
    std::vector<VkAttachmentReference> inputAttachments;
    std::vector<VkAttachmentReference> colorAttachments;
    std::optional<VkAttachmentReference> resolveAttachment;
    std::vector<VkAttachmentReference> preserveAttachments;
};

struct SubpassDependency
{

};

struct RenderpassSpecification
{
    std::vector<VkAttachmentDescription> attachments;
    std::vector<SubpassSpecification> subpasses;
    std::vector<VkSubpassDependency> dependencies;
    std::vector<VkImageView> imageViews;
};

class VulkanRenderpass
{

};


#endif //VULKANRENDERINGENGINE_VULKAN_RENDERPASS_HPP
