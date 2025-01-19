//
// Created by Gianni on 11/01/2025.
//

#include "instancedMesh.hpp"

static constexpr uint32_t sInitialInstanceBufferCapacity = 32;

InstancedMesh::InstancedMesh()
{
    init();
}

InstancedMesh::InstancedMesh(const VulkanRenderDevice &renderDevice, uint32_t vertexCount,
                             const InstancedMesh::Vertex *vertexData, uint32_t indexCount,
                             const uint32_t *indexData, const std::string &name)
{
    init();
    create(renderDevice, vertexCount, vertexData, indexCount, indexData, name);
}

void InstancedMesh::create(const VulkanRenderDevice& renderDevice,
                           uint32_t vertexCount,
                           const Vertex* vertexData,
                           uint32_t indexCount,
                           const uint32_t* indexData,
                           const std::string& name)
{
    mVertexBuffer = createBufferWithStaging(renderDevice,
                                            vertexCount * sizeof(Vertex),
                                            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                                VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                            vertexData);

    mIndexBuffer.buffer = createBufferWithStaging(renderDevice,
                                                  indexCount * sizeof(uint32_t),
                                                  VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                                      VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                  indexData);
    mIndexBuffer.count = indexCount;

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        mInstanceBuffers[i] = createBuffer(renderDevice,
                                              sInitialInstanceBufferCapacity * sizeof(InstanceData),
                                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                                  VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                  VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }

    mName = name;
    tag(renderDevice);
}

void InstancedMesh::destroy(const VulkanRenderDevice &renderDevice)
{
    destroyBuffer(renderDevice, mVertexBuffer);
    destroyBuffer(renderDevice, mIndexBuffer.buffer);
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        destroyBuffer(renderDevice, mInstanceBuffers[i]);
}

// add to all
uint32_t InstancedMesh::addInstance(const InstancedMesh::InstanceData &instanceData, uint32_t frameIndex)
{
    uint32_t instanceID = mInstanceIdCounter++;
    ++mInstanceCount;
    mInstanceIdMap[instanceID] = mInstanceCount;

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        uint32_t currentFrameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
        mAddPending[currentFrameIndex].push_back(instanceData);
    }

    return instanceID;
}

void InstancedMesh::updateInstance(const InstancedMesh::InstanceData &instanceData, uint32_t instanceID, uint32_t frameIndex)
{
    std::pair<InstanceData, uint32_t> pair(instanceData, instanceID);

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        uint32_t currentFrameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;

        if (mUpdatePending[currentFrameIndex].contains(pair))
            mUpdatePending[currentFrameIndex].erase(pair);
        else
            mUpdatePending[currentFrameIndex].insert(pair);
    }
}

void InstancedMesh::removeInstance(uint32_t instanceID, uint32_t frameIndex)
{
    --mInstanceCount;

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        uint32_t currentFrameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
        mRemovePending[currentFrameIndex].insert(instanceID);
    }

    assert(mInstanceCount >= 0);
}

void InstancedMesh::updateInstanceData(const VulkanRenderDevice& renderDevice, VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
    if (mInstanceBufferCapacity[frameIndex] < mInstanceCount)
        resize(renderDevice, commandBuffer, frameIndex);

    addInstances(renderDevice, commandBuffer, frameIndex);
    updateInstances(renderDevice, commandBuffer, frameIndex);
    removeInstances(renderDevice, commandBuffer, frameIndex);

    uint32_t nextFrame = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
    mAddPending[nextFrame].clear();
    mUpdatePending[nextFrame].clear();
    mRemovePending[nextFrame].clear();
}

void InstancedMesh::render(VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
    VkBuffer buffers[2] {
        mVertexBuffer.buffer,
        mInstanceBuffers[frameIndex].buffer
    };

    VkDeviceSize offsets[2] {0, 0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 2, buffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, mIndexBuffer.buffer.buffer, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, mIndexBuffer.count, mInstanceCount, 0, 0, 0);
}

void InstancedMesh::tag(const VulkanRenderDevice& renderDevice)
{
    setDebugVulkanObjectName(renderDevice.device,
                             VK_OBJECT_TYPE_BUFFER,
                             std::format("{} mesh vertex buffer", mName),
                             mVertexBuffer.buffer);

    setDebugVulkanObjectName(renderDevice.device,
                             VK_OBJECT_TYPE_BUFFER,
                             std::format("{} mesh index buffer", mName),
                             mIndexBuffer.buffer.buffer);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        setDebugVulkanObjectName(renderDevice.device,
                                 VK_OBJECT_TYPE_BUFFER,
                                 std::format("{} mesh instance buffer {}", mName, i),
                                 mInstanceBuffers[i].buffer);
    }
}

std::array<VkVertexInputBindingDescription, 2> constexpr InstancedMesh::bindingDescriptions()
{
    VkVertexInputBindingDescription vertexBindingDescription {
        .binding = 0,
        .stride = sizeof(InstancedMesh::Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    VkVertexInputBindingDescription instanceBindingDescription {
        .binding = 1,
        .stride = sizeof(InstancedMesh::InstanceData),
        .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
    };

    return {
        vertexBindingDescription,
        instanceBindingDescription
    };
}

std::array<VkVertexInputAttributeDescription, 15> constexpr InstancedMesh::attributeDescriptions()
{
    std::array<VkVertexInputAttributeDescription, 15> attributeDescriptions;

    // Vertex::position
    attributeDescriptions.at(0).location = 0;
    attributeDescriptions.at(0).binding = 0;
    attributeDescriptions.at(0).format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions.at(0).offset = offsetof(InstancedMesh::Vertex, position);

    // Vertex::texCoords
    attributeDescriptions.at(1).location = 1;
    attributeDescriptions.at(1).binding = 0;
    attributeDescriptions.at(1).format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions.at(1).offset = offsetof(InstancedMesh::Vertex, texCoords);

    // Vertex::normal
    attributeDescriptions.at(2).location = 2;
    attributeDescriptions.at(2).binding = 0;
    attributeDescriptions.at(2).format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions.at(2).offset = offsetof(InstancedMesh::Vertex, normal);

    // Vertex::tangent
    attributeDescriptions.at(3).location = 3;
    attributeDescriptions.at(3).binding = 0;
    attributeDescriptions.at(3).format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions.at(3).offset = offsetof(InstancedMesh::Vertex, tangent);

    // Vertex::bitangent
    attributeDescriptions.at(4).location = 4;
    attributeDescriptions.at(4).binding = 0;
    attributeDescriptions.at(4).format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions.at(4).offset = offsetof(InstancedMesh::Vertex, bitangent);

    for (size_t i = 0; i < 4; ++i)
    {
        // PerInstanceData::modelMatrix
        attributeDescriptions.at(5 + i).location = 5 + i;
        attributeDescriptions.at(5 + i).binding = 1;
        attributeDescriptions.at(5 + i).format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions.at(5 + i).offset = offsetof(InstancedMesh::InstanceData, modelMatrix) + i * sizeof(glm::vec4);

        // PerInstanceData::normalMatrix
        attributeDescriptions.at(5 + 4 + i).location = 5 + 4 + i;
        attributeDescriptions.at(5 + 4 + i).binding = 1;
        attributeDescriptions.at(5 + 4 + i).format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions.at(5 + 4 + i).offset = offsetof(InstancedMesh::InstanceData, normalMatrix) + i * sizeof(glm::vec4);
    }

    // PerInstanceData::id
    attributeDescriptions.at(13).location = 13;
    attributeDescriptions.at(13).binding = 1;
    attributeDescriptions.at(13).format = VK_FORMAT_R32_UINT;
    attributeDescriptions.at(13).offset = offsetof(InstancedMesh::InstanceData, id);

    // PerInstanceData::materialIndex
    attributeDescriptions.at(14).location = 14;
    attributeDescriptions.at(14).binding = 1;
    attributeDescriptions.at(14).format = VK_FORMAT_R32_UINT;
    attributeDescriptions.at(14).offset = offsetof(InstancedMesh::InstanceData, materialIndex);

    return attributeDescriptions;
}

void InstancedMesh::init()
{
    mVertexBuffer = {};
    mIndexBuffer = {};
    mInstanceBuffers = {};

    mInstanceCount = 0;
    mInstanceIdCounter = 0;
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        mInstanceBufferCapacity[i] = sInitialInstanceBufferCapacity;
}

void InstancedMesh::addInstances(const VulkanRenderDevice &renderDevice, VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
    uint32_t addCount = mAddPending[frameIndex].size();

    if (!addCount)
        return;

    uint32_t stagingBufferSize = addCount * sizeof(InstanceData);

    VulkanBuffer stagingBuffer = createBuffer(renderDevice,
                                             stagingBufferSize,
                                             VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                             VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    mapBufferMemory(renderDevice, stagingBuffer, 0, stagingBufferSize, mAddPending[frameIndex].data());


    VkBufferCopy copyRegion {
        .srcOffset {},
        .dstOffset = (mInstanceCount - addCount) * sizeof(InstanceData),
        .size = stagingBufferSize
    };

    vkCmdCopyBuffer(commandBuffer, mInstanceBuffers[frameIndex].buffer, stagingBuffer.buffer, 1, &copyRegion);

    destroyBuffer(renderDevice, stagingBuffer);
}

void InstancedMesh::updateInstances(const VulkanRenderDevice &renderDevice, VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
    uint32_t updateCount = mUpdatePending[frameIndex].size();

    if (!updateCount)
        return;

    std::vector<InstanceData> data(updateCount);
    std::vector<VkBufferCopy> copyRegions(updateCount);

    data.reserve(updateCount);
    copyRegions.reserve(updateCount);

    uint32_t i = 0;
    for (const auto& [instanceData, instanceID] : mUpdatePending[frameIndex])
    {
        uint32_t instanceIndex = mInstanceIdMap.at(instanceID);

        VkBufferCopy copyRegion {
            .srcOffset = i * sizeof(InstanceData),
            .dstOffset = instanceIndex * sizeof(InstanceData),
            .size = sizeof(InstanceData)
        };

        data.push_back(instanceData);
        copyRegions.push_back(copyRegion);

        ++i;
    }

    VkDeviceSize size = updateCount * sizeof(InstanceData);
    VulkanBuffer stagingBuffer = createBuffer(renderDevice,
                                              size,
                                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                                  VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    mapBufferMemory(renderDevice, stagingBuffer, 0, size, data.data());

    vkCmdCopyBuffer(commandBuffer,
                    stagingBuffer.buffer,
                    mInstanceBuffers[frameIndex].buffer,
                    copyRegions.size(),
                    copyRegions.data());
}

// todo: handle case when there is only one instance or if the instance removed is the last
void InstancedMesh::removeInstances(const VulkanRenderDevice &renderDevice, VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
    uint32_t removeCount = mRemovePending[frameIndex].size();

    if (!removeCount)
        return;

    std::vector<VkBufferCopy> copyRegions;
    copyRegions.reserve(removeCount);

    uint32_t moveIndexBegin = mInstanceCount - removeCount;
    uint32_t i = 0;
    for (uint32_t instanceID : mRemovePending[frameIndex])
    {
        uint32_t moveIndex = moveIndexBegin + i;
        uint32_t removeIndex = mInstanceIdMap.at(instanceID);

        VkBufferCopy copyRegion {
            .srcOffset = moveIndex * sizeof(InstanceData),
            .dstOffset = removeIndex * sizeof(InstanceData),
            .size = sizeof(InstanceData)
        };

        copyRegions.push_back(copyRegion);

        mInstanceIdMap.at(instanceID) = removeIndex;
    }

    vkCmdCopyBuffer(commandBuffer,
                    mInstanceBuffers[frameIndex].buffer,
                    mInstanceBuffers[frameIndex].buffer,
                    copyRegions.size(),
                    copyRegions.data());
}

void InstancedMesh::resize(const VulkanRenderDevice &renderDevice, VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
    assert(mInstanceCount <= mInstanceBufferCapacity[frameIndex]);

    VulkanBuffer newBuffer = createBuffer(renderDevice,
                                          sizeof(InstanceData) * glm::ceil(glm::log2(static_cast<float>(mInstanceCount))),
                                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                              VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                              VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    VkBufferCopy copyRegion {
        .srcOffset {},
        .dstOffset {},
        .size = mInstanceBufferCapacity[frameIndex] * sizeof(InstanceData)
    };

    vkCmdCopyBuffer(commandBuffer, mInstanceBuffers[frameIndex].buffer, newBuffer.buffer, 1, &copyRegion);

    destroyBuffer(renderDevice, mInstanceBuffers[frameIndex]);

    mInstanceBuffers[frameIndex] = newBuffer;

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        setDebugVulkanObjectName(renderDevice.device,
                                 VK_OBJECT_TYPE_BUFFER,
                                 std::format("{} mesh instance buffer {}", mName, i),
                                 mInstanceBuffers[i].buffer);
    }
}

const std::string &InstancedMesh::name() const
{
    return mName;
}
