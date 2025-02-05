//
// Created by Gianni on 23/01/2025.
//

#ifndef VULKANRENDERINGENGINE_LOADED_RESOURCE_HPP
#define VULKANRENDERINGENGINE_LOADED_RESOURCE_HPP

#include "../vk/vulkan_texture.hpp"
#include "../renderer/instanced_mesh.hpp"
#include "../renderer/bounding_box.hpp"
#include "../renderer/material.hpp"
#include "../renderer/model.hpp"
#include "../renderer/vertex.hpp"

struct MeshData
{
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::optional<index_t> materialIndex;
};

struct LoadedModelData
{
    struct Node
    {
        std::string name;
        glm::mat4 transformation;
        std::optional<index_t> meshIndex;
        std::vector<Node> children;
    };

    struct Mesh
    {
        std::string name;
        std::shared_ptr<InstancedMesh> mesh;
        std::optional<index_t> materialIndex;
    };

    struct Material
    {
        std::string name;
        int32_t baseColorTexIndex = -1;
        int32_t metallicRoughnessTexIndex = -1;
        int32_t normalTexIndex = -1;
        int32_t aoTexIndex = -1;
        int32_t emissionTexIndex = -1;
        glm::vec4 baseColorFactor = glm::vec4(1.f);
        glm::vec4 emissionColorFactor = glm::vec4(0.f);
        float metallicFactor = 1.f;
        float roughnessFactor = 1.f;
        float occlusionFactor = 1.f;
    };

    std::filesystem::path path;
    std::string name;
    Node root;
    BoundingBox bb;
    std::vector<Mesh> meshes;
    std::vector<Material> materials;
    std::vector<std::pair<std::shared_ptr<VulkanTexture>, std::filesystem::path>> textures;
    std::unordered_map<int32_t, uint32_t> indirectTextureMap;

    uint32_t getTextureIndex(int32_t matTexIndex) const
    {
        return indirectTextureMap.at(matTexIndex);
    }
};

#endif //VULKANRENDERINGENGINE_LOADED_RESOURCE_HPP
