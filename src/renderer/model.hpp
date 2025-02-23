//
// Created by Gianni on 13/01/2025.
//

#ifndef VULKANRENDERINGENGINE_MODEL_HPP
#define VULKANRENDERINGENGINE_MODEL_HPP

#include <glm/glm.hpp>
#include "../app/simple_notification_service.hpp"
#include "../app/uuid_registry.hpp"
#include "../vk/vulkan_texture.hpp"
#include "../vk/vulkan_descriptor.hpp"
#include "instanced_mesh.hpp"
#include "material.hpp"

struct SceneNode
{
    std::string name;
    glm::vec3 translation;
    glm::vec3 rotation;
    glm::vec3 scale;
    std::vector<uint32_t> meshIndices;
    std::vector<SceneNode> children;
};

struct Mesh
{
    uuid32_t meshID;
    std::string name;
    InstancedMesh mesh;
    uint32_t materialIndex;
};

struct Texture
{
    std::string name;
    VulkanTexture vulkanTexture;
    VkDescriptorSet descriptorSet;
};

class Model : public SubscriberSNS
{
public:
    uuid32_t id;
    std::string name;
    std::filesystem::path path;

    SceneNode root;
    std::vector<Texture> textures;
    std::vector<Material> materials;
    std::vector<std::string> materialNames;
    std::vector<Mesh> meshes;

    VkCullModeFlags cullMode;
    VkFrontFace frontFace;

public:
    Model(const VulkanRenderDevice& renderDevice);
    ~Model();

    void updateMaterial(index_t matIndex);

    void notify(const Message &message) override;

    void createMaterialsUBO();
    void createTextureDescriptorSets(VkDescriptorSetLayout dsLayout);

    void render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t matDsIndex) const;

    Mesh* getMesh(uuid32_t meshID);

private:
    void bindMaterialUBO(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t materialIndex, uint32_t matDsIndex) const;
    void bindTextures(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t materialIndex, uint32_t matDsIndex) const;

private:
    const VulkanRenderDevice& mRenderDevice;
    VulkanBuffer mMaterialsUBO;
};

const char* toStr(VkCullModeFlags cullMode);
const char* toStr(VkFrontFace frontFace);

#endif //VULKANRENDERINGENGINE_MODEL_HPP
