//
// Created by Gianni on 13/01/2025.
//

#ifndef VULKANRENDERINGENGINE_MODEL_HPP
#define VULKANRENDERINGENGINE_MODEL_HPP

#include <glm/glm.hpp>
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
    glm::vec3 center;
};

struct Texture
{
    std::string name;
    VulkanTexture vulkanTexture;
    VkDescriptorSet descriptorSet;
};

class Model
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
    Model();
    Model(const VulkanRenderDevice& renderDevice);
    ~Model();

    Model(const Model&) = delete;
    Model& operator=(const Model&) = delete;

    Model(Model&& other) noexcept;
    Model& operator=(Model&& other) noexcept;

    void updateMaterial(index_t matIndex);

    void createMaterialsUBO();
    void createTextureDescriptorSets(VkDescriptorSetLayout dsLayout);

    void bindMaterialUBO(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t materialIndex, uint32_t matDsIndex) const;
    void bindTextures(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout, uint32_t materialIndex, uint32_t matDsIndex) const;

    bool drawOpaque(const Mesh& mesh) const;
    Mesh* getMesh(uuid32_t meshID);
    void swap(Model& other);

private:
    const VulkanRenderDevice* mRenderDevice;
    VulkanBuffer mMaterialsUBO;
};

const char* toStr(VkCullModeFlags cullMode);
const char* toStr(VkFrontFace frontFace);

#endif //VULKANRENDERINGENGINE_MODEL_HPP
