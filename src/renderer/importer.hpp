//
// Created by Gianni on 13/01/2025.
//

#ifndef VULKANRENDERINGENGINE_IMPORTER_HPP
#define VULKANRENDERINGENGINE_IMPORTER_HPP

#include <glm/glm.hpp>
#include "model.hpp"

struct LoadedMeshData
{
    size_t modelId;
    std::string meshName;
    std::vector<Mesh::Vertex> vertices;
    std::vector<uint32_t> indices;
    glm::mat4 transformation;
};

std::unique_ptr<aiScene> loadScene(const std::string &path);

std::vector<Mesh::Vertex> loadMeshVertices(const aiMesh& mesh);

std::vector<uint32_t> loadMeshIndices(const aiMesh& mesh);

glm::mat4 assimpToGlmMat4(const aiMatrix4x4& mat);

#endif //VULKANRENDERINGENGINE_IMPORTER_HPP
