//
// Created by Gianni on 11/01/2025.
//

#ifndef VULKANRENDERINGENGINE_INSTANCED_MESH_HPP
#define VULKANRENDERINGENGINE_INSTANCED_MESH_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include "../vk/vulkan_render_device.hpp"
#include "../vk/vulkan_buffer.hpp"
#include "../app/types.hpp"
#include "vertex.hpp"

class InstancedMesh
{
public:
    struct InstanceData
    {
        glm::mat4 modelMatrix;
        glm::mat3 normalMatrix;
        uint32_t id;
    };

public:
    InstancedMesh();
    InstancedMesh(const VulkanRenderDevice& renderDevice,
                  const std::vector<Vertex>& vertices,
                  const std::vector<uint32_t>& indices);

    uint32_t addInstance();
    void updateInstance(uint32_t instanceID, const glm::mat4& transformation, uuid32_t id);
    void removeInstance(uint32_t instanceID);
    void setDebugName(const std::string& debugName);
    void render(VkCommandBuffer commandBuffer) const;

    static constexpr std::array<VkVertexInputBindingDescription, 2> bindingDescriptions();
    static constexpr std::array<VkVertexInputAttributeDescription, 14> attributeDescriptions();

private:
    void checkResize();

private:
    const VulkanRenderDevice* mRenderDevice;

    VulkanBuffer mVertexBuffer;
    VulkanBuffer mIndexBuffer;
    VulkanBuffer mInstanceBuffer;

    uint32_t mInstanceCount;
    uint32_t mInstanceBufferCapacity;

    std::unordered_map<uint32_t, uint32_t> mInstanceIdToIndexMap;
    std::unordered_map<uint32_t, uint32_t> mInstanceIndexToIdMap;
};

#endif //VULKANRENDERINGENGINE_INSTANCED_MESH_HPP
