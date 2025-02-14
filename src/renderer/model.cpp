//
// Created by Gianni on 19/01/2025.
//

#include "model.hpp"

Model::Model(const VulkanRenderDevice* renderDevice)
    : SubscriberSNS({Topic::Type::SceneGraph})
    , mRenderDevice(renderDevice)
    , mMaterialsDS()
{
}

Model::~Model()
{
    vkFreeDescriptorSets(mRenderDevice->device, mRenderDevice->descriptorPool, 1, &mMaterialsDS);
    for (auto& texture : textures)
    {
        vkFreeDescriptorSets(mRenderDevice->device,
                             mRenderDevice->descriptorPool,
                             1, &texture.descriptorSet);
    }
}

void Model::createMaterialsUBO()
{
    mMaterialsUBO = VulkanBuffer(*mRenderDevice,
                                 sizeof(Material) * materials.size(),
                                 BufferType::Uniform,
                                 MemoryType::GPU);
    mMaterialsUBO.setDebugName(name + " Material UBO");
}

void Model::createTextureDescriptorSets(VkDescriptorSetLayout dsLayout)
{
    VkDescriptorSetAllocateInfo dsAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = mRenderDevice->descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &dsLayout
    };

    for (auto& texture : textures)
    {
        VkResult result = vkAllocateDescriptorSets(mRenderDevice->device, &dsAllocateInfo, &texture.descriptorSet);
        vulkanCheck(result, "Failed to allocate texture descriptor set.");

        setDSDebugName(*mRenderDevice, texture.descriptorSet, texture.name);

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

        vkUpdateDescriptorSets(mRenderDevice->device, 1, &dsWrite, 0, nullptr);
    }
}

void Model::createMaterialsDescriptorSet(VkDescriptorSetLayout dsLayout)
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
        .descriptorPool = mRenderDevice->descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &dsLayout
    };

    VkResult result = vkAllocateDescriptorSets(mRenderDevice->device, &descriptorSetAllocateInfo, &mMaterialsDS);
    vulkanCheck(result, "Failed to create mMaterialsDS.");
    setDSDebugName(*mRenderDevice, mMaterialsDS, name + " Materials DS");
}

void Model::render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t matDsIndex) const
{
    vkCmdBindDescriptorSets(commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout,
                            matDsIndex, 1, &mMaterialsDS,
                            0, nullptr);

    for (const auto& mesh : meshes)
    {
        vkCmdPushConstants(commandBuffer,
                           pipelineLayout,
                           VK_SHADER_STAGE_FRAGMENT_BIT,
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

void Model::notify(const Message &message)
{
    if (const auto m = message.getIf<Message::MeshInstanceUpdate>())
    {
        Mesh& mesh = getMesh(m->meshID);
        mesh.mesh.updateInstance(m->objectID, m->transformation);
    }

    if (const auto m = message.getIf<Message::RemoveMeshInstance>())
    {
        Mesh& mesh = getMesh(m->meshID);
        mesh.mesh.removeInstance(m->objectID);
    }
}
