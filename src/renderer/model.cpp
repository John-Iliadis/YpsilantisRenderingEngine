//
// Created by Gianni on 19/01/2025.
//

#include "model.hpp"

Model::Model()
    : mMaterialsDS()
{
}

void Model::createMaterialsUBO(const VulkanRenderDevice &renderDevice)
{
    mMaterialsUBO = VulkanBuffer(renderDevice,
                                 sizeof(Material) * materials.size(),
                                 BufferType::Uniform,
                                 MemoryType::GPU);
    mMaterialsUBO.setDebugName(name + " Material UBO");
}

void Model::createTextureDescriptorSets(const VulkanRenderDevice &renderDevice, VkDescriptorSetLayout dsLayout)
{
    VkDescriptorSetAllocateInfo dsAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = renderDevice.descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &dsLayout
    };

    for (auto& texture : textures)
    {
        VkResult result = vkAllocateDescriptorSets(renderDevice.device, &dsAllocateInfo, &texture.descriptorSet);
        vulkanCheck(result, "Failed to allocate texture descriptor set.");

        setDSDebugName(renderDevice, texture.descriptorSet, texture.name);

        VkDescriptorImageInfo imageInfo {
            .sampler = texture.vulkanTexture.vulkanSampler.sampler,
            .imageView = texture.vulkanTexture.vulkanImage.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        VkWriteDescriptorSet dsWrite {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = texture.descriptorSet,
            .dstBinding = 0,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &imageInfo
        };

        vkUpdateDescriptorSets(renderDevice.device, 1, &dsWrite, 0, nullptr);
    }
}

void Model::createMaterialsDescriptorSet(const VulkanRenderDevice &renderDevice, VkDescriptorSetLayout dsLayout)
{
    uint32_t descriptorCounts = PerModelMaxTextureCount;

    VkDescriptorSetVariableDescriptorCountAllocateInfoEXT variableDescriptorCountAllocInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
        .descriptorSetCount = 1,
        .pDescriptorCounts = &descriptorCounts
    };

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = &variableDescriptorCountAllocInfo,
        .descriptorPool = renderDevice.descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &dsLayout
    };

    VkResult result = vkAllocateDescriptorSets(renderDevice.device, &descriptorSetAllocateInfo, &mMaterialsDS);
    vulkanCheck(result, "Failed to create mMaterialsDS.");
    setDSDebugName(renderDevice, mMaterialsDS, name + " Materials DS");
}

// todo: set cull face
void Model::render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout)
{
    vkCmdBindDescriptorSets(commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout,
                            0, 1, &mMaterialsDS,
                            0, nullptr);

    for (const auto& mesh : meshes)
    {
        vkCmdPushConstants(commandBuffer,
                           pipelineLayout,
                           VK_SHADER_STAGE_VERTEX_BIT,
                           0, sizeof(int32_t),
                           &mesh.materialIndex);
        mesh.mesh.render(commandBuffer);
    }
}

Mesh &Model::getMesh(uuid32_t meshID)
{
    for (auto& mesh : meshes)
        if (mesh.meshID == meshID)
            return mesh;
    return meshes.front();
}

void Model::updateMaterial(index_t matIndex)
{
    mMaterialsUBO.update(matIndex * sizeof(Material), sizeof(Material), &materials.at(matIndex));
}
