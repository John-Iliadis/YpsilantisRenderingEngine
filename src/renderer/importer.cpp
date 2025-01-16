//
// Created by Gianni on 13/01/2025.
//

#include "importer.hpp"

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
    aiComponent_CAMERAS |
    aiComponent_COLORS
};

static constexpr int sRemovePrimitives
{
    aiPrimitiveType_POINT |
    aiPrimitiveType_LINE
};

//void Importer::importModel(const std::filesystem::path& filepath)
//{
//    LoadedModel model {};
//    model.model.name = filepath.filename().string();
//
//    mLoadedModelFutures[model.model.name] = std::async(std::launch::async,
//                                                       [path = filepath.string()] () {
//        Assimp::Importer importer;
//        importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, sRemoveComponents);
//        importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, sRemovePrimitives);
//        importer.SetPropertyBool(AI_CONFIG_PP_PTV_NORMALIZE, true);
//
//        const aiScene* scene = importer.ReadFile(path, sImportFlags);
//
//        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
//            check(false, std::format("Failed to load model {}", path).c_str());
//    });
//}
//
//std::unique_ptr<aiScene> loadScene(const std::string &path)
//{
//    Assimp::Importer importer;
//    importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, sRemoveComponents);
//    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, sRemovePrimitives);
//    importer.SetPropertyBool(AI_CONFIG_PP_PTV_NORMALIZE, true);
//
//    importer.ReadFile(path, sImportFlags);
//    aiScene* scene = importer.GetOrphanedScene();
//
//    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
//        check(false, std::format("Failed to load model {}", path).c_str());
//
//    return std::unique_ptr<aiScene>(scene);
//}
//
//std::vector<Mesh::Vertex> Importer::loadMeshVertices(const aiMesh &mesh)
//{
//    std::vector<Mesh::Vertex> vertices(mesh.mNumVertices);
//
//    for (size_t i = 0; i < vertices.size(); ++i)
//    {
//        vertices.at(i).position = *reinterpret_cast<glm::vec3*>(&mesh.mVertices[i]);
//        vertices.at(i).normal = *reinterpret_cast<glm::vec3*>(&mesh.mNormals[i]);
//        vertices.at(i).tangent = *reinterpret_cast<glm::vec3*>(&mesh.mTangents[i]);
//        vertices.at(i).bitangent = *reinterpret_cast<glm::vec3*>(&mesh.mBitangents[i]);
//        vertices.at(i).texCoords = *reinterpret_cast<glm::vec2*>(&mesh.mTextureCoords[0][i]);
//    }
//
//    return vertices;
//}
//
//std::vector<uint32_t> Importer::loadMeshIndices(const aiMesh &mesh)
//{
//    aiMaterial
//    std::vector<uint32_t> loadMeshIndices(const aiMesh& mesh)
//    {
//        std::vector<uint32_t> indices(mesh.mNumFaces * 3);
//
//        for (uint32_t i = 0; i < mesh.mNumFaces; ++i)
//        {
//            const aiFace& face = mesh.mFaces[i];
//
//            indices.at(i) = face.mIndices[0];
//            indices.at(i + 1) = face.mIndices[1];
//            indices.at(i + 2) = face.mIndices[2];
//        }
//
//        return indices;
//    }
//}
//
//glm::mat4 Importer::assimpToGlmMat4(const aiMatrix4x4 &mat)
//{
//    return {
//        mat.a1, mat.b1, mat.c1, mat.d1,
//        mat.a2, mat.b2, mat.c2, mat.d2,
//        mat.a3, mat.b3, mat.c3, mat.d3,
//        mat.a4, mat.b4, mat.c4, mat.d4
//    };
//}
