//
// Created by Gianni on 11/01/2025.
//

#include "mesh.hpp"

static constexpr uint32_t sInitialInstanceBufferCapacity = 32;

Mesh::Mesh()
    : mVertexBuffer()
    , mIndexBuffer()
    , mInstanceBuffers()
    , mInstanceCount()
    , mInstanceBufferCapacity(sInitialInstanceBufferCapacity)
    , mInstanceIdCounter()
    , mDebugName()
{
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        mInstanceBufferCapacity[i] = sInitialInstanceBufferCapacity;
}

void Mesh::create(const VulkanRenderDevice &renderDevice,
                  VkDeviceSize vertexBufferSize,
                  const Vertex* vertexData,
                  VkDeviceSize indexBufferSize,
                  const uint32_t* indexData)
{
    mVertexBuffer = createBufferWithStaging(renderDevice,
                                            vertexBufferSize,
                                           VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                               VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                            vertexData);

    mIndexBuffer.buffer = createBufferWithStaging(renderDevice,
                                                  indexBufferSize,
                                                 VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                                     VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                                  VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                                                  indexData);
    mIndexBuffer.count = indexBufferSize / sizeof(uint32_t);

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        mInstanceBuffers[i] = createBuffer(renderDevice,
                                              sInitialInstanceBufferCapacity * sizeof(InstanceData),
                                              VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                                 VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                                                 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                              VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    }
}

void Mesh::destroy(const VulkanRenderDevice &renderDevice)
{
    destroyBuffer(renderDevice, mVertexBuffer);
    destroyBuffer(renderDevice, mIndexBuffer.buffer);
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        destroyBuffer(renderDevice, mInstanceBuffers[i]);
}

// add to all
uint32_t Mesh::addInstance(const Mesh::InstanceData &instanceData, uint32_t frameIndex)
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

void Mesh::updateInstance(const Mesh::InstanceData &instanceData, uint32_t instanceID, uint32_t frameIndex)
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

void Mesh::removeInstance(uint32_t instanceID, uint32_t frameIndex)
{
    --mInstanceCount;

    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        uint32_t currentFrameIndex = (frameIndex + 1) % MAX_FRAMES_IN_FLIGHT;
        mRemovePending[currentFrameIndex].insert(instanceID);
    }

    assert(mInstanceCount >= 0);
}

void Mesh::updateInstanceData(const VulkanRenderDevice& renderDevice, VkCommandBuffer commandBuffer, uint32_t frameIndex)
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

void Mesh::render(VkCommandBuffer commandBuffer, uint32_t frameIndex)
{
    VkBuffer buffers[2] {
        mVertexBuffer.buffer,
        mInstanceBuffers[frameIndex].buffer
    };

    VkDeviceSize offsets[2] {0, 0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 2, buffers, offsets);
    vkCmdBindIndexBuffer(commandBuffer, mIndexBuffer.buffer.buffer, 0, VK_INDEX_TYPE_UINT32);

//    void vkCmdDrawIndexed(VkCommandBuffer commandBuffer,
//                          uint32_t indexCount,
//                          uint32_t instanceCount,
//                          uint32_t firstIndex,
//                          int32_t vertexOffset,
//                          uint32_t firstInstance);
}

void Mesh::tag(const VulkanRenderDevice& renderDevice, const char* meshName)
{
    mDebugName = meshName;

    setDebugVulkanObjectName(renderDevice.device,
                             VK_OBJECT_TYPE_BUFFER,
                             std::format("{} vertex buffer", meshName),
                             mVertexBuffer.buffer);

    setDebugVulkanObjectName(renderDevice.device,
                             VK_OBJECT_TYPE_BUFFER,
                             std::format("{} index buffer", meshName),
                             mIndexBuffer.buffer.buffer);

    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
    {
        setDebugVulkanObjectName(renderDevice.device,
                                 VK_OBJECT_TYPE_BUFFER,
                                 std::format("{} instance buffer {}", meshName, i),
                                 mInstanceBuffers[i].buffer);
    }
}

std::array<VkVertexInputBindingDescription, 2> constexpr Mesh::bindingDescriptions()
{
    VkVertexInputBindingDescription vertexBindingDescription {
        .binding = 0,
        .stride = sizeof(Mesh::Vertex),
        .inputRate = VK_VERTEX_INPUT_RATE_VERTEX
    };

    VkVertexInputBindingDescription instanceBindingDescription {
        .binding = 1,
        .stride = sizeof(Mesh::InstanceData),
        .inputRate = VK_VERTEX_INPUT_RATE_INSTANCE
    };

    return {
        vertexBindingDescription,
        instanceBindingDescription
    };
}

std::array<VkVertexInputAttributeDescription, 15> constexpr Mesh::attributeDescriptions()
{
    std::array<VkVertexInputAttributeDescription, 15> attributeDescriptions;

    // Vertex::position
    attributeDescriptions.at(0).location = 0;
    attributeDescriptions.at(0).binding = 0;
    attributeDescriptions.at(0).format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions.at(0).offset = offsetof(Mesh::Vertex, position);

    // Vertex::texCoords
    attributeDescriptions.at(1).location = 1;
    attributeDescriptions.at(1).binding = 0;
    attributeDescriptions.at(1).format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions.at(1).offset = offsetof(Mesh::Vertex, texCoords);

    // Vertex::normal
    attributeDescriptions.at(2).location = 2;
    attributeDescriptions.at(2).binding = 0;
    attributeDescriptions.at(2).format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions.at(2).offset = offsetof(Mesh::Vertex, normal);

    // Vertex::tangent
    attributeDescriptions.at(3).location = 3;
    attributeDescriptions.at(3).binding = 0;
    attributeDescriptions.at(3).format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions.at(3).offset = offsetof(Mesh::Vertex, tangent);

    // Vertex::bitangent
    attributeDescriptions.at(4).location = 4;
    attributeDescriptions.at(4).binding = 0;
    attributeDescriptions.at(4).format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions.at(4).offset = offsetof(Mesh::Vertex, bitangent);

    for (size_t i = 0; i < 4; ++i)
    {
        // PerInstanceData::modelMatrix
        attributeDescriptions.at(5 + i).location = 5 + i;
        attributeDescriptions.at(5 + i).binding = 1;
        attributeDescriptions.at(5 + i).format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions.at(5 + i).offset = offsetof(Mesh::InstanceData, modelMatrix) + i * sizeof(glm::vec4);

        // PerInstanceData::normalMatrix
        attributeDescriptions.at(5 + 4 + i).location = 5 + 4 + i;
        attributeDescriptions.at(5 + 4 + i).binding = 1;
        attributeDescriptions.at(5 + 4 + i).format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions.at(5 + 4 + i).offset = offsetof(Mesh::InstanceData, normalMatrix) + i * sizeof(glm::vec4);
    }

    // PerInstanceData::id
    attributeDescriptions.at(13).location = 13;
    attributeDescriptions.at(13).binding = 1;
    attributeDescriptions.at(13).format = VK_FORMAT_R32_UINT;
    attributeDescriptions.at(13).offset = offsetof(Mesh::InstanceData, id);

    // PerInstanceData::materialIndex
    attributeDescriptions.at(14).location = 14;
    attributeDescriptions.at(14).binding = 1;
    attributeDescriptions.at(14).format = VK_FORMAT_R32_UINT;
    attributeDescriptions.at(14).offset = offsetof(Mesh::InstanceData, materialIndex);

    return attributeDescriptions;
}

void Mesh::addInstances(const VulkanRenderDevice &renderDevice, VkCommandBuffer commandBuffer, uint32_t frameIndex)
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

void Mesh::updateInstances(const VulkanRenderDevice &renderDevice, VkCommandBuffer commandBuffer, uint32_t frameIndex)
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

void Mesh::removeInstances(const VulkanRenderDevice &renderDevice, VkCommandBuffer commandBuffer, uint32_t frameIndex)
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

void Mesh::resize(const VulkanRenderDevice &renderDevice, VkCommandBuffer commandBuffer, uint32_t frameIndex)
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

    if (mDebugName)
    {
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
        {
            setDebugVulkanObjectName(renderDevice.device,
                                     VK_OBJECT_TYPE_BUFFER,
                                     std::format("{} instance buffer {}", mDebugName, i),
                                     mInstanceBuffers[i].buffer);
        }
    }
}
