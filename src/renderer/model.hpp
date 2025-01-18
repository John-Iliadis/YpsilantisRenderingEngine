//
// Created by Gianni on 13/01/2025.
//

#ifndef VULKANRENDERINGENGINE_MODEL_HPP
#define VULKANRENDERINGENGINE_MODEL_HPP

#include "material.hpp"
#include "instancedMesh.hpp"

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
