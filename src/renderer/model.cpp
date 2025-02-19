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

void Model::createMaterialsSSBO()
{
    mMaterialsSSBO = VulkanBuffer(*mRenderDevice,
                                 sizeof(Material) * materials.size(),
                                  BufferType::Storage,
                                  MemoryType::GPU);
    mMaterialsSSBO.setDebugName(name + " Material SSBO");
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

        VkDescriptorImageInfo imageInfo {
            .sampler = texture.vulkanTexture.vulkanSampler.sampler,
            .imageView = texture.vulkanTexture.imageView,
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

    VkDescriptorBufferInfo materialBufferInfo {
        .buffer = mMaterialsSSBO.getBuffer(),
        .offset = 0,
        .range = VK_WHOLE_SIZE
    };

    std::vector<VkDescriptorImageInfo> imageInfos(textures.size());
    for (uint32_t i = 0; i < imageInfos.size(); ++i)
    {
        imageInfos.at(i).sampler = textures.at(i).vulkanTexture.vulkanSampler.sampler;
        imageInfos.at(i).imageView = textures.at(i).vulkanTexture.imageView;
        imageInfos.at(i).imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    VkWriteDescriptorSet materialsDsWrite {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = mMaterialsDS,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        .pBufferInfo = &materialBufferInfo
    };

    VkWriteDescriptorSet imagesDsWrite {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = mMaterialsDS,
        .dstBinding = 1,
        .dstArrayElement = 0,
        .descriptorCount = static_cast<uint32_t >(imageInfos.size()),
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = imageInfos.data()
    };

    std::array<VkWriteDescriptorSet, 2> descriptorWrites {
        materialsDsWrite,
        imagesDsWrite
    };

    vkUpdateDescriptorSets(mRenderDevice->device, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
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
    mMaterialsSSBO.update(matIndex * sizeof(Material), sizeof(Material), &materials.at(matIndex));
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
