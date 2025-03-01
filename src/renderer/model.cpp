//
// Created by Gianni on 19/01/2025.
//

#include "model.hpp"

Model::Model(const VulkanRenderDevice& renderDevice)
    : SubscriberSNS({Topic::Type::SceneGraph})
    , mRenderDevice(renderDevice)
    , cullMode(VK_CULL_MODE_BACK_BIT)
    , frontFace(VK_FRONT_FACE_COUNTER_CLOCKWISE)
{
}

Model::~Model()
{
    for (auto& texture : textures)
    {
        vkFreeDescriptorSets(mRenderDevice.device,
                             mRenderDevice.descriptorPool,
                             1, &texture.descriptorSet);
    }
}

void Model::createMaterialsUBO()
{
    mMaterialsUBO = VulkanBuffer(mRenderDevice,
                                 sizeof(Material) * materials.size(),
                                 BufferType::Uniform,
                                 MemoryType::HostCoherent,
                                 materials.data());
    mMaterialsUBO.setDebugName(name + " Material UBO");

    readBufferToVector<Material>(mRenderDevice.device, mMaterialsUBO.getMemory(), sizeof(Material));
}

void Model::createTextureDescriptorSets(VkDescriptorSetLayout dsLayout)
{
    VkDescriptorSetAllocateInfo dsAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = mRenderDevice.descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &dsLayout
    };

    for (auto& texture : textures)
    {
        VkResult result = vkAllocateDescriptorSets(mRenderDevice.device, &dsAllocateInfo, &texture.descriptorSet);
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

        vkUpdateDescriptorSets(mRenderDevice.device, 1, &dsWrite, 0, nullptr);
    }
}

Mesh* Model::getMesh(uuid32_t meshID)
{
    for (auto& mesh : meshes)
        if (mesh.meshID == meshID)
            return &mesh;

    return nullptr;
}

void Model::updateMaterial(index_t matIndex)
{
    mMaterialsUBO.mapBufferMemory(matIndex * sizeof(Material), sizeof(Material), &materials.at(matIndex));
}

void Model::notify(const Message &message)
{
    if (const auto m = message.getIf<Message::MeshInstanceUpdate>())
    {
        if (auto mesh = getMesh(m->meshID))
        {
            mesh->mesh.updateInstance(m->objectID, m->transformation);
        }
    }

    if (const auto m = message.getIf<Message::RemoveMeshInstance>())
    {
        if (auto mesh = getMesh(m->meshID))
        {
            mesh->mesh.removeInstance(m->objectID);
        }
    }
}

void Model::bindMaterialUBO(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t materialIndex, uint32_t matDsIndex) const
{
    VkDescriptorBufferInfo bufferInfo {
        .buffer = mMaterialsUBO.getBuffer(),
        .offset = materialIndex * sizeof(Material),
        .range = sizeof(Material)
    };

    VkWriteDescriptorSet writeDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = VK_NULL_HANDLE,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .pBufferInfo = &bufferInfo
    };

    pfnCmdPushDescriptorSet(commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout,
                            matDsIndex,
                            1,
                            &writeDescriptorSet);
}

void Model::bindTextures(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t materialIndex, uint32_t matDsIndex) const
{
    const Material& mat = materials.at(materialIndex);

    int32_t baseColorTexIndex = mat.baseColorTexIndex;
    int32_t metallicTexIndex = mat.metallicTexIndex;
    int32_t roughnessTexIndex = mat.roughnessTexIndex;
    int32_t normalTexIndex = mat.normalTexIndex;
    int32_t aoTexIndex = mat.aoTexIndex;
    int32_t emissionTexIndex = mat.emissionTexIndex;

    auto imageInfo = [this] (int32_t texIndex, const VulkanTexture& def) {
        VkDescriptorImageInfo descriptorImageInfo {
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        if (texIndex == -1)
        {
            descriptorImageInfo.sampler = def.vulkanSampler.sampler;
            descriptorImageInfo.imageView = def.imageView;

            return descriptorImageInfo;
        }

        const auto& tex = textures.at(texIndex).vulkanTexture;

        descriptorImageInfo.sampler = tex.vulkanSampler.sampler;
        descriptorImageInfo.imageView = tex.imageView;

        return descriptorImageInfo;
    };

    VkDescriptorImageInfo baseColorImageInfo = imageInfo(baseColorTexIndex, defaultBaseColorTex);
    VkDescriptorImageInfo metallicImageInfo = imageInfo(metallicTexIndex, defaultMetallicTex);
    VkDescriptorImageInfo roughnessImageInfo = imageInfo(roughnessTexIndex, defaultRoughnessTex);
    VkDescriptorImageInfo normalImageInfo = imageInfo(normalTexIndex, defaultNormalTex);
    VkDescriptorImageInfo aoImageInfo = imageInfo(aoTexIndex, defaultAoTex);
    VkDescriptorImageInfo emissionImageInfo = imageInfo(emissionTexIndex, defaultEmissionTex);

    VkWriteDescriptorSet prototypeWriteDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = VK_NULL_HANDLE,
        .dstBinding = 1,
        .dstArrayElement = 0,
        .descriptorCount = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = nullptr
    };

    std::array<VkWriteDescriptorSet, 6> descriptorWrites;
    descriptorWrites.fill(prototypeWriteDescriptorSet);

    descriptorWrites.at(0).dstBinding = 1;
    descriptorWrites.at(0).pImageInfo = &baseColorImageInfo;

    descriptorWrites.at(1).dstBinding = 2;
    descriptorWrites.at(1).pImageInfo = &metallicImageInfo;

    descriptorWrites.at(2).dstBinding = 3;
    descriptorWrites.at(2).pImageInfo = &roughnessImageInfo;

    descriptorWrites.at(3).dstBinding = 4;
    descriptorWrites.at(3).pImageInfo = &normalImageInfo;

    descriptorWrites.at(4).dstBinding = 5;
    descriptorWrites.at(4).pImageInfo = &aoImageInfo;

    descriptorWrites.at(5).dstBinding = 6;
    descriptorWrites.at(5).pImageInfo = &emissionImageInfo;

    pfnCmdPushDescriptorSet(commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS,
                            pipelineLayout,
                            matDsIndex,
                            descriptorWrites.size(),
                            descriptorWrites.data());
}

bool Model::drawOpaque(const Mesh &mesh)
{
    return mesh.opaque && materials.at(mesh.materialIndex).baseColorFactor.a == 1.f;
}

const char* toStr(VkCullModeFlags cullMode)
{
    switch (cullMode)
    {
        case VK_CULL_MODE_NONE: return "None";
        case VK_CULL_MODE_BACK_BIT: return "Back";
        case VK_CULL_MODE_FRONT_BIT: return "Front";
        case VK_CULL_MODE_FRONT_AND_BACK: return "Front and Back";
        default: return "Unknown";
    }
}

const char* toStr(VkFrontFace frontFace)
{
    switch (frontFace)
    {
        case VK_FRONT_FACE_CLOCKWISE: return "Clockwise";
        case VK_FRONT_FACE_COUNTER_CLOCKWISE: return "Counter Clockwise";
        default: return "Unknown";
    }
}
