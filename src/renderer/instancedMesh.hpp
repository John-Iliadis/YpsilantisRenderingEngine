//
// Created by Gianni on 11/01/2025.
//

#ifndef VULKANRENDERINGENGINE_INSTANCEDMESH_HPP
#define VULKANRENDERINGENGINE_INSTANCEDMESH_HPP

#include <glm/glm.hpp>
#include "../vk/include.hpp"

class InstancedMesh
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
    InstancedMesh();
    InstancedMesh(const VulkanRenderDevice& renderDevice,
                  uint32_t vertexCount, const Vertex* vertexData,
                  uint32_t indexCount, const uint32_t* indexData,
                  const std::string& name);

    void create(const VulkanRenderDevice& renderDevice,
                uint32_t vertexCount, const Vertex* vertexData,
                uint32_t indexCount, const uint32_t* indexData,
                const std::string& name);
    void destroy(const VulkanRenderDevice& renderDevice);

    uint32_t addInstance(const InstanceData& instanceData, uint32_t frameIndex);
    void updateInstance(const InstanceData& instanceData, uint32_t instanceID, uint32_t frameIndex);
    void removeInstance(uint32_t instanceID, uint32_t frameIndex);

    void updateInstanceData(const VulkanRenderDevice& renderDevice, VkCommandBuffer commandBuffer, uint32_t frameIndex);

    void render(VkCommandBuffer commandBuffer, uint32_t frameIndex);

    const std::string& name() const;

    static constexpr std::array<VkVertexInputBindingDescription, 2> bindingDescriptions();
    static constexpr std::array<VkVertexInputAttributeDescription, 15> attributeDescriptions();

private:
    void init();
    void addInstances(const VulkanRenderDevice& renderDevice, VkCommandBuffer commandBuffer, uint32_t frameIndex);
    void updateInstances(const VulkanRenderDevice& renderDevice, VkCommandBuffer commandBuffer, uint32_t frameIndex);
    void removeInstances(const VulkanRenderDevice& renderDevice, VkCommandBuffer commandBuffer, uint32_t frameIndex);
    void resize(const VulkanRenderDevice& renderDevice, VkCommandBuffer commandBuffer, uint32_t frameIndex);
    void tag(const VulkanRenderDevice& renderDevice);

private:
    std::string mName;

    VulkanBuffer mVertexBuffer;
    IndexBuffer mIndexBuffer;
    std::array<VulkanBuffer, MAX_FRAMES_IN_FLIGHT> mInstanceBuffers;

    std::unordered_map<uint32_t, uint32_t> mInstanceIdMap; // instanceID to instanceIndex
    uint32_t mInstanceCount;
    uint32_t mInstanceIdCounter;

    std::array<uint32_t, MAX_FRAMES_IN_FLIGHT> mInstanceBufferCapacity;
    std::array<std::vector<InstanceData>, MAX_FRAMES_IN_FLIGHT> mAddPending;
    std::array<std::unordered_set<std::pair<InstanceData, uint32_t>, UpdateInstanceHash, UpdateInstanceCompare>,
               MAX_FRAMES_IN_FLIGHT> mUpdatePending;
    std::array<std::unordered_set<uint32_t>, MAX_FRAMES_IN_FLIGHT> mRemovePending;
};

#endif //VULKANRENDERINGENGINE_INSTANCEDMESH_HPP
