//
// Created by Gianni on 13/01/2025.
//

#include "model.hpp"

static constexpr uint32_t sImportFlags
{
    aiProcess_Triangulate |
    aiProcess_GenNormals |
    aiProcess_GenUVCoords |
    aiProcess_CalcTangentSpace |
    aiProcess_JoinIdenticalVertices |
    aiProcess_RemoveComponent |
    aiProcess_SortByPType
};

static constexpr int sRemoveComponents
{
    aiComponent_BONEWEIGHTS |
    aiComponent_ANIMATIONS |
    aiComponent_LIGHTS |
    aiComponent_CAMERAS
};

static constexpr int sRemovePrimitives
{
    aiPrimitiveType_POINT |
    aiPrimitiveType_LINE
};

void Model::create(const VulkanRenderDevice& renderDevice,
                   const std::string& path,
                   uint32_t importFlags)
{
    debugLog(std::format("Loading model {} ...", path));

    Assimp::Importer importer;
    importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, sRemoveComponents);
    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, sRemovePrimitives);
    // importer.SetPropertyBool(AI_CONFIG_PP_PTV_NORMALIZE, true);

    const aiScene* scene = importer.ReadFile(path, importFlags | sImportFlags);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        check(false, std::format("Failed to load model {}", path).c_str());

    processNode(renderDevice, scene->mRootNode, scene);
}

void Model::destroy(const VulkanRenderDevice &renderDevice)
{
    for (MeshNode& mesh : mMeshNodes)
        mesh.mesh.destroy(renderDevice);
}

void Model::processNode(const VulkanRenderDevice& renderDevice, aiNode* node, const aiScene* scene)
{
    for (uint32_t i = 0; i < node->mNumMeshes; ++i)
    {
        uint32_t meshIndex = node->mMeshes[i];
        aiMesh* assimpMesh = scene->mMeshes[meshIndex];

        MeshNode meshNode;
        meshNode.mesh = createMesh(renderDevice, assimpMesh, scene);
        meshNode.transformation = *reinterpret_cast<glm::mat4*>(&node->mTransformation.Transpose());

        mMeshNodes.push_back(meshNode);
    }

    for (uint32_t i = 0; i < node->mNumChildren; ++i)
        processNode(renderDevice, node->mChildren[i], scene);
}

Mesh Model::createMesh(const VulkanRenderDevice& renderDevice, aiMesh* mesh, const aiScene* scene)
{
    std::vector<Mesh::Vertex> vertices = getVertices(mesh);
    std::vector<uint32_t> indices = getIndices(mesh);

    Mesh mesh_;
    mesh_.create(renderDevice, vertices.size(), vertices.data(), indices.size(), indices.data());

    return mesh_;
}

std::vector<Mesh::Vertex> Model::getVertices(aiMesh *mesh)
{
    std::vector<Mesh::Vertex> vertices(mesh->mNumVertices);

    for (size_t i = 0; i < vertices.size(); ++i)
    {
        vertices.at(i).position = *reinterpret_cast<glm::vec3*>(&mesh->mVertices[i]);
        vertices.at(i).normal = *reinterpret_cast<glm::vec3*>(&mesh->mNormals[i]);
        vertices.at(i).tangent = *reinterpret_cast<glm::vec3*>(&mesh->mTangents[i]);
        vertices.at(i).bitangent = *reinterpret_cast<glm::vec3*>(&mesh->mBitangents[i]);
        vertices.at(i).texCoords = *reinterpret_cast<glm::vec2*>(&mesh->mTextureCoords[0][i]);
    }

    return vertices;
}

std::vector<uint32_t> Model::getIndices(aiMesh *mesh)
{
    std::vector<uint32_t> indices(mesh->mNumFaces * 3);

    for (uint32_t i = 0; i < mesh->mNumFaces; ++i)
    {
        const aiFace& face = mesh->mFaces[i];

        indices.at(i) = face.mIndices[0];
        indices.at(i + 1) = face.mIndices[1];
        indices.at(i + 2) = face.mIndices[2];
    }

    return indices;
}
