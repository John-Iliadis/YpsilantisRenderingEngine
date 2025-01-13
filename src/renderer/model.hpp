//
// Created by Gianni on 13/01/2025.
//

#ifndef VULKANRENDERINGENGINE_MODEL_HPP
#define VULKANRENDERINGENGINE_MODEL_HPP

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "mesh.hpp"

class Model
{
public:
    struct MeshNode
    {
        glm::mat4 transformation;
        Mesh mesh;
    };

    void create(const VulkanRenderDevice& renderDevice,
                const std::string& path,
                uint32_t importFlags);
    void destroy(const VulkanRenderDevice& renderDevice);

private:
    void processNode(const VulkanRenderDevice& renderDevice, aiNode* node, const aiScene* scene);
    Mesh createMesh(const VulkanRenderDevice& renderDevice, aiMesh* mesh, const aiScene* scene);
    std::vector<Mesh::Vertex> getVertices(aiMesh* mesh);
    std::vector<uint32_t> getIndices(aiMesh* mesh);

private:
    std::vector<MeshNode> mMeshNodes;
};


#endif //VULKANRENDERINGENGINE_MODEL_HPP
