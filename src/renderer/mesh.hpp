//
// Created by Gianni on 11/01/2025.
//

#ifndef VULKANRENDERINGENGINE_MESH_HPP
#define VULKANRENDERINGENGINE_MESH_HPP

#include <glm/glm.hpp>
#include "../vk/include.hpp"

class Mesh
{
public:
    struct Vertex
    {
        glm::vec3 position;
        glm::vec2 texCoords;
        glm::vec3 normal;
        glm::vec3 tangent;
        glm::vec3 bitangent;
    };

    struct InstanceData
    {
        glm::mat4 modelMatrix;
        glm::mat4 normalMatrix;
        uint32_t id;
        uint32_t materialIndex;
    };

    struct UpdateInstanceHash
    {
        size_t operator()(const std::pair<InstanceData, uint32_t>& pair) const
        {
            return static_cast<size_t>(pair.second);
        }
    };

    struct UpdateInstanceCompare
    {
        bool operator()(const std::pair<InstanceData, uint32_t>& pair1,
                        const std::pair<InstanceData, uint32_t>& pair2) const
        {
            return pair1.second == pair2.second;
        }
    };

public:
    Mesh();

    void create(const VulkanRenderDevice& renderDevice,
                VkDeviceSize vertexBufferSize,
                const Vertex* vertexData,
                VkDeviceSize indexBufferSize,
                const uint32_t* indexData);
    void destroy(const VulkanRenderDevice& renderDevice);

    uint32_t addInstance(const InstanceData& instanceData, uint32_t frameIndex);
    void updateInstance(const InstanceData& instanceData, uint32_t instanceID, uint32_t frameIndex);
    void removeInstance(uint32_t instanceID, uint32_t frameIndex);

    void updateInstanceData(const VulkanRenderDevice& renderDevice, VkCommandBuffer commandBuffer, uint32_t frameIndex);

    void render(VkCommandBuffer commandBuffer, uint32_t frameIndex);

    void tag(const VulkanRenderDevice& renderDevice, const char* meshName);

    static constexpr std::array<VkVertexInputBindingDescription, 2> bindingDescriptions();
    static constexpr std::array<VkVertexInputAttributeDescription, 15> attributeDescriptions();

private:
    void addInstances(const VulkanRenderDevice& renderDevice, VkCommandBuffer commandBuffer, uint32_t frameIndex);
    void updateInstances(const VulkanRenderDevice& renderDevice, VkCommandBuffer commandBuffer, uint32_t frameIndex);
    void removeInstances(const VulkanRenderDevice& renderDevice, VkCommandBuffer commandBuffer, uint32_t frameIndex);
    void resize(const VulkanRenderDevice& renderDevice, VkCommandBuffer commandBuffer, uint32_t frameIndex);

private:
    VulkanBuffer mVertexBuffer;
    IndexBuffer mIndexBuffer;
    VulkanBuffer mInstanceBuffers[MAX_FRAMES_IN_FLIGHT];

    std::unordered_map<uint32_t, uint32_t> mInstanceIdMap; // instanceID to instanceIndex
    uint32_t mInstanceCount;
    uint32_t mInstanceIdCounter;

    uint32_t mInstanceBufferCapacity[MAX_FRAMES_IN_FLIGHT];
    std::vector<InstanceData> mAddPending[MAX_FRAMES_IN_FLIGHT];
    std::unordered_set<std::pair<InstanceData, uint32_t>,
        UpdateInstanceHash,
        UpdateInstanceCompare> mUpdatePending[MAX_FRAMES_IN_FLIGHT];
    std::unordered_set<uint32_t> mRemovePending[MAX_FRAMES_IN_FLIGHT];

    const char* mDebugName;
};


#endif //VULKANRENDERINGENGINE_MESH_HPP
