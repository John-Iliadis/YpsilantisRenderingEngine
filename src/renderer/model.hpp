//
// Created by Gianni on 13/01/2025.
//

#ifndef VULKANRENDERINGENGINE_MODEL_HPP
#define VULKANRENDERINGENGINE_MODEL_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "scene.hpp"
#include "material.hpp"
#include "instancedMesh.hpp"

using MappedMaterialName = std::string;
using TextureName = std::string;

struct ModelMesh
{
    std::shared_ptr<InstancedMesh> mesh;
    MappedMaterialName mappedMaterialName;

    ModelMesh();
    ModelMesh(const MappedMaterialName& materialName);
};

struct ModelNode
{
    std::string name;
    glm::mat4 transformation;
    std::vector<size_t> meshIndices;
    std::vector<ModelNode> children;

    ModelNode();
    ModelNode(const std::string& name, const glm::mat4& transformation);
};

class Model
{
public:
    std::string name;
    ModelNode root;
    std::vector<ModelMesh> meshes;
    std::unordered_map<MappedMaterialName, std::shared_ptr<NamedMaterial>> materialsMapped;

public:
    Model();
    Model(const std::string& name);
};

#endif //VULKANRENDERINGENGINE_MODEL_HPP
