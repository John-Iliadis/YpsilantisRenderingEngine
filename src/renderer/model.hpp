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
#include "bounding_box.hpp"

struct Material
{
    alignas(4) int32_t baseColorTexIndex;
    alignas(4) int32_t metallicRoughnessTexIndex;
    alignas(4) int32_t normalTexIndex;
    alignas(4) int32_t aoTexIndex;
    alignas(4) int32_t emissionTexIndex;
    alignas(4) float metallicFactor;
    alignas(4) float roughnessFactor;
    alignas(4) float occlusionFactor;
    alignas(16) glm::vec4 baseColorFactor;
    alignas(16) glm::vec4 emissionFactor;
};

struct SceneNode
{
    std::string name;
    glm::mat4 transformation;
    int32_t meshIndex;
    std::vector<SceneNode> children;
};

struct Mesh
{
    uuid64_t meshID;
    InstancedMesh mesh;
    int32_t materialIndex;
};

struct Texture
{
    std::string name;
    VulkanTexture vulkanTexture;
    VkDescriptorSet descriptorSet; // todo: free these
};

class Model : public SubscriberSNS
{
public:
    std::string name;
    std::vector<SceneNode> scenes;
    std::vector<Texture> textures;
    std::vector<Material> materials;
    std::vector<Mesh> meshes;

public:
    Model();
    ~Model() = default;

    void createMaterialsUBO(const VulkanRenderDevice& renderDevice);
    void createTextureDescriptorSets(const VulkanRenderDevice& renderDevice, VkDescriptorSetLayout dsLayout);
    void createMaterialsDescriptorSet(const VulkanRenderDevice& renderDevice, VkDescriptorSetLayout dsLayout);

    void render(VkCommandBuffer commandBuffer, VkPipelineLayout pipelineLayout);

private:
    VkDescriptorSet mMaterialsDS;
    VulkanBuffer mMaterialsUBO;
};


#endif //VULKANRENDERINGENGINE_MODEL_HPP
