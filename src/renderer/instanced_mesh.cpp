//
// Created by Gianni on 11/01/2025.
//

#include "instanced_mesh.hpp"

static constexpr uint32_t sInitialInstanceBufferCapacity = 32;
static constexpr uint32_t sVertexSize = sizeof(Vertex);
static constexpr uint32_t sInstanceSize = sizeof(InstancedMesh::InstanceData);

InstancedMesh::InstancedMesh()
    : mRenderDevice()
    , mInstanceCount()
    , mInstanceBufferCapacity()
{
}

InstancedMesh::InstancedMesh(const VulkanRenderDevice &renderDevice,
                             const std::vector<Vertex> &vertices,
                             const std::vector<uint32_t> &indices)
    : mRenderDevice(&renderDevice)
    , mVertexBuffer(renderDevice, vertices.size() * sVertexSize, BufferType::Vertex, MemoryType::GPU, vertices.data())
    , mIndexBuffer(renderDevice, indices.size() * sizeof(uint32_t), BufferType::Index, MemoryType::GPU, indices.data())
    , mInstanceBuffer(renderDevice, sInitialInstanceBufferCapacity * sInstanceSize, BufferType::Vertex, MemoryType::GPU)
    , mInstanceCount()
    , mInstanceBufferCapacity(sInitialInstanceBufferCapacity)
{
}

InstancedMesh::InstancedMesh(const VulkanRenderDevice &renderDevice,
                             VulkanBuffer&& vertexBuffer,
                             VulkanBuffer&& indexBuffer)
    : mRenderDevice(&renderDevice)
    , mVertexBuffer(std::move(vertexBuffer))
    , mIndexBuffer(std::move(indexBuffer))
    , mInstanceBuffer(renderDevice, sInitialInstanceBufferCapacity * sInstanceSize, BufferType::Vertex, MemoryType::GPU)
    , mInstanceCount()
    , mInstanceBufferCapacity(sInitialInstanceBufferCapacity)
{
}

void InstancedMesh::addInstance(uuid32_t id)
{
    checkResize();

    ++mInstanceCount;

    uint32_t instanceIndex = mInstanceCount - 1;

    assert(mInstanceIdToIndexMap.emplace(id, instanceIndex).second);
    assert(mInstanceIndexToIdMap.emplace(instanceIndex, id).second);

    InstanceData instanceData {.id = id};
    mInstanceBuffer.update(instanceIndex * sInstanceSize, sInstanceSize, &instanceData);
}

void InstancedMesh::updateInstance(uuid32_t id, const glm::mat4& transformation)
{
    uint32_t instanceIndex = mInstanceIdToIndexMap.at(id);

    InstanceData instanceData {
        .modelMatrix = transformation,
        .normalMatrix = glm::inverseTranspose(glm::mat3(transformation)),
        .id = id,
    };

    mInstanceBuffer.update(instanceIndex * sInstanceSize, sInstanceSize, &instanceData);
}

void InstancedMesh::removeInstance(uuid32_t id)
{
    uint32_t removeIndex = mInstanceIdToIndexMap.at(id);

    mInstanceIdToIndexMap.erase(id);
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
    vkCmdDrawIndexed(commandBuffer, getIndexCount(mIndexBuffer), mInstanceCount, 0, 0, 0);
}

std::vector<VkVertexInputBindingDescription> InstancedMesh::bindingDescriptions()
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

std::vector<VkVertexInputAttributeDescription> InstancedMesh::attributeDescriptions()
{
    static auto vertAttrib = [] (uint32_t loc, uint32_t binding, VkFormat format, uint32_t offset) {
        VkVertexInputAttributeDescription vertexInputAttributeDescription {
            .location = loc,
            .binding = binding,
            .format = format,
            .offset = offset
        };

        return vertexInputAttributeDescription;
    };

    return {
        vertAttrib(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)),
        vertAttrib(1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoords)),
        vertAttrib(2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)),
        vertAttrib(3, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, tangent)),
        vertAttrib(4, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, bitangent)),
        vertAttrib(5, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(InstanceData, modelMatrix)),
        vertAttrib(6, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(InstanceData, modelMatrix) + sizeof(glm::vec4)),
        vertAttrib(7, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(InstanceData, modelMatrix) + 2 * sizeof(glm::vec4)),
        vertAttrib(8, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(InstanceData, modelMatrix) + 3 * sizeof(glm::vec4)),
        vertAttrib(9, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(InstanceData, normalMatrix)),
        vertAttrib(10, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(InstanceData, normalMatrix) + sizeof(glm::vec4)),
        vertAttrib(11, 1, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(InstanceData, normalMatrix) + 2 * sizeof(glm::vec4)),
        vertAttrib(12, 1, VK_FORMAT_R32_UINT, offsetof(InstanceData, id)),
    };
}

void InstancedMesh::checkResize()
{
    if (mInstanceCount < mInstanceBufferCapacity)
        return;

    uint32_t newCapacity = mInstanceCount * 2;

    VulkanBuffer newInstanceBuffer(*mRenderDevice, newCapacity * sInstanceSize, BufferType::Vertex, MemoryType::GPU);
    newInstanceBuffer.copyBuffer(mInstanceBuffer, 0, 0, mInstanceBufferCapacity * sInstanceSize);
    newInstanceBuffer.swap(mInstanceBuffer);

    mInstanceBufferCapacity = newCapacity;
}
