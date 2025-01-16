//
// Created by Gianni on 13/01/2025.
//

#ifndef VULKANRENDERINGENGINE_MODEL_HPP
#define VULKANRENDERINGENGINE_MODEL_HPP

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "material.hpp"
#include "mesh.hpp"

//struct Model
//{
//    struct MeshNode
//    {
//        size_t materialId;
//        glm::mat4 transformation;
//    };
//
//    size_t id;
//    std::string name;
//    std::vector<size_t> meshes;
//    std::unordered_map<size_t, MeshNode> meshNodeMap; // mesh id to mesh node graph data
//};

struct Model
{
    struct MeshNode
    {
        std::string_view meshName;
        std::string_view materialName;
        glm::mat4 localTransform;
        std::vector<MeshNode> children;
    };

    std::string name;
    MeshNode meshRootNode;
    std::unordered_map<std::string, std::string_view> mappedMaterials;
};

#endif //VULKANRENDERINGENGINE_MODEL_HPP
