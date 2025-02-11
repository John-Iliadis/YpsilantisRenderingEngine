//
// Created by Gianni on 11/01/2025.
//

#include "instanced_mesh.hpp"

static uint32_t generateInstanceID()
{
    static uint32_t counter = 0;
    return counter++;
}

static constexpr uint32_t sInitialInstanceBufferCapacity = 32;
static constexpr uint32_t sVertexSize = sizeof(Vertex);
static constexpr uint32_t sInstanceSize = sizeof(InstancedMesh::InstanceData);

InstancedMesh::InstancedMesh()
    : mRenderDevice()
    , mInstanceCount()
    , mInstanceBufferCapacity()
{
}

// todo: check for bugs
InstancedMesh::InstancedMesh(const VulkanRenderDevice &renderDevice,
                             const std::vector<Vertex> &vertices,
                             const std::vector<uint32_t> &indices)
    : mRenderDevice(&renderDevice)
    , mVertexBuffer(renderDevice, vertices.size() * sVertexSize, BufferType::Vertex, MemoryType::GPU)
    , mIndexBuffer(renderDevice, indices.size() * sizeof(uint32_t), BufferType::Index, MemoryType::GPU)
    , mInstanceBuffer(renderDevice, sInitialInstanceBufferCapacity * sInstanceSize, BufferType::Vertex, MemoryType::GPU)
    , mInstanceCount()
    , mInstanceBufferCapacity(sInitialInstanceBufferCapacity)
{
}

uint32_t InstancedMesh::addInstance()
{
    checkResize();

    ++mInstanceCount;

    uint32_t instanceID = generateInstanceID();
    uint32_t instanceIndex = mInstanceCount - 1;

    assert(mInstanceIdToIndexMap.emplace(instanceID, instanceIndex).second);
    assert(mInstanceIndexToIdMap.emplace(instanceIndex, instanceID).second);

    InstanceData placeholderData;
    mInstanceBuffer.update(instanceIndex * sInstanceSize, sInstanceSize, &placeholderData);

    return instanceID;
}

void InstancedMesh::updateInstance(uint32_t instanceID, const glm::mat4& model, uuid32_t id)
{
    uint32_t instanceIndex = mInstanceIdToIndexMap.at(instanceID);

    InstanceData instanceData {
        .modelMatrix = model,
        .normalMatrix = glm::inverseTranspose(glm::mat3(model)),
        .id = id,
    };

    mInstanceBuffer.update(instanceIndex * sInstanceSize, sInstanceSize, &instanceData);
}

void InstancedMesh::removeInstance(uint32_t instanceID)
{
    uint32_t removeIndex = mInstanceIdToIndexMap.at(instanceID);

    mInstanceIdToIndexMap.erase(instanceID);
    mInstanceIndexToIdMap.erase(removeIndex);

    if (removeIndex != mInstanceCount - 1)
    {
        uint32_t transferIndex = mInstanceCount - 1;
        uint32_t transferIndexID = mInstanceIndexToIdMap.at(transferIndex);

        mInstanceIdToIndexMap.at(transferIndexID) = removeIndex;
        mInstanceIndexToIdMap.emplace(removeIndex, transferIndex);

        mInstanceBuffer.copyBuffer(mInstanceBuffer,
                                   removeIndex * sInstanceSize,
                                   transferIndex * sInstanceSize,
                                   sInstanceSize);
    }

    --mInstanceCount;
}

void InstancedMesh::setDebugName(const std::string &debugName)
{
    mVertexBuffer.setDebugName(debugName);
    mIndexBuffer.setDebugName(debugName);
    mIndexBuffer.setDebugName(debugName);
}

void InstancedMesh::render(VkCommandBuffer commandBuffer) const
{
    if (mInstanceCount == 0)
        return;

    VkBuffer buffers[2] {
        mVertexBuffer.getBuffer(),
        mInstanceBuffer.getBuffer()
    };

    VkDeviceSize offsets[2] {0, 0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 2, buffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, mIndexBuffer.getBuffer(), 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, getIndexCount(mInstanceBuffer), mInstanceCount, 0, 0, 0);
}

std::array<VkVertexInputBindingDescription, 2> constexpr InstancedMesh::bindingDescriptions()
{
    VkVertexInputBindingDescription vertexBindingDescription {
        .binding = 0,
        .stride = sVertexSize,
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    VkVertexInputBindingDescription instanceBindingDescription {
        .binding = 1,
        .stride = sInstanceSize,
        .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
    };

    return {
        vertexBindingDescription,
        instanceBindingDescription
    };
}

std::array<VkVertexInputAttributeDescription, 14> constexpr InstancedMesh::attributeDescriptions()
{
    std::array<VkVertexInputAttributeDescription, 14> attributeDescriptions;

    // Vertex::position
    attributeDescriptions.at(0).location = 0;
    attributeDescriptions.at(0).binding = 0;
    attributeDescriptions.at(0).format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions.at(0).offset = offsetof(Vertex, position);

    // Vertex::texCoords
    attributeDescriptions.at(1).location = 1;
    attributeDescriptions.at(1).binding = 0;
    attributeDescriptions.at(1).format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions.at(1).offset = offsetof(Vertex, texCoords);

    // Vertex::normal
    attributeDescriptions.at(2).location = 2;
    attributeDescriptions.at(2).binding = 0;
    attributeDescriptions.at(2).format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions.at(2).offset = offsetof(Vertex, normal);

    // Vertex::tangent
    attributeDescriptions.at(3).location = 3;
    attributeDescriptions.at(3).binding = 0;
    attributeDescriptions.at(3).format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions.at(3).offset = offsetof(Vertex, tangent);

    // Vertex::bitangent
    attributeDescriptions.at(4).location = 4;
    attributeDescriptions.at(4).binding = 0;
    attributeDescriptions.at(4).format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions.at(4).offset = offsetof(Vertex, bitangent);

    // PerInstanceData::modelMatrix
    for (size_t i = 0; i < 4; ++i)
    {
        attributeDescriptions.at(5 + i).location = 5 + i;
        attributeDescriptions.at(5 + i).binding = 1;
        attributeDescriptions.at(5 + i).format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions.at(5 + i).offset = offsetof(InstanceData, modelMatrix) + i * sizeof(glm::vec4);
    }

    // PerInstanceData::normalMatrix
    for (size_t i = 0; i < 3; ++i)
    {
        attributeDescriptions.at(9 + i).location = 9 + i;
        attributeDescriptions.at(9 + i).binding = 1;
        attributeDescriptions.at(9 + i).format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions.at(9 + i).offset = offsetof(InstanceData, normalMatrix) + i * sizeof(glm::vec4);
    }

    // PerInstanceData::id
    attributeDescriptions.at(12).location = 13;
    attributeDescriptions.at(12).binding = 1;
    attributeDescriptions.at(12).format = VK_FORMAT_R32_UINT;
    attributeDescriptions.at(12).offset = offsetof(InstanceData, id);

    return attributeDescriptions;
}

void InstancedMesh::checkResize()
{
    if (mInstanceCount < mInstanceBufferCapacity)
        return;

    uint32_t newCapacity = glm::ceil(glm::log2(static_cast<float>(mInstanceCount)));

    VulkanBuffer newInstanceBuffer(*mRenderDevice, newCapacity * sizeof(sInstanceSize), BufferType::Vertex, MemoryType::GPU);
    newInstanceBuffer.copyBuffer(mInstanceBuffer, 0, 0, mInstanceBufferCapacity * sInstanceSize);
    newInstanceBuffer.swap(mInstanceBuffer);

    mInstanceBufferCapacity = newCapacity;
}
