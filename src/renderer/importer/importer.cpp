//
// Created by Gianni on 13/01/2025.
//

#include "importer.hpp"
#include <stb/stb_image.h>

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

namespace Importer
{
    std::vector<::Mesh::Vertex> loadMeshVertices(const aiMesh& mesh)
    {
        std::vector<::Mesh::Vertex> vertices(mesh.mNumVertices);

        for (size_t i = 0; i < vertices.size(); ++i)
        {
            vertices.at(i).position = *reinterpret_cast<glm::vec3*>(&mesh.mVertices[i]);
            vertices.at(i).normal = *reinterpret_cast<glm::vec3*>(&mesh.mNormals[i]);
            vertices.at(i).tangent = *reinterpret_cast<glm::vec3*>(&mesh.mTangents[i]);
            vertices.at(i).bitangent = *reinterpret_cast<glm::vec3*>(&mesh.mBitangents[i]);
            vertices.at(i).texCoords = *reinterpret_cast<glm::vec2*>(&mesh.mTextureCoords[0][i]);
        }

        return vertices;
    }

    std::vector<uint32_t> loadMeshIndices(const aiMesh& mesh)
    {
        std::vector<uint32_t> indices(mesh.mNumFaces * 3);

        for (uint32_t i = 0; i < mesh.mNumFaces; ++i)
        {
            const aiFace& face = mesh.mFaces[i];

            indices.at(i) = face.mIndices[0];
            indices.at(i + 1) = face.mIndices[1];
            indices.at(i + 2) = face.mIndices[2];
        }

        return indices;
    }

    glm::mat4 assimpToGlmMat4(const aiMatrix4x4& mat)
    {
        return {
            mat.a1, mat.b1, mat.c1, mat.d1,
            mat.a2, mat.b2, mat.c2, mat.d2,
            mat.a3, mat.b3, mat.c3, mat.d3,
            mat.a4, mat.b4, mat.c4, mat.d4
        };
    }

    void ModelImporter::enqueueTask(Importer::Task task)
    {
        std::lock_guard<std::mutex> lock(mTaskQueueMutex);
        mTaskQueue.push_back(std::move(task));
    }

    void ModelImporter::processMainThread()
    {
        std::lock_guard<std::mutex> lock(mTaskQueueMutex);
        for (const auto& task : mTaskQueue)
            task();
        mTaskQueue.clear();
    }

    void ModelImporter::importModel(const std::filesystem::path &filepath, const VulkanRenderDevice &renderDevice)
    {
        Scene scene {
            .name = filepath.filename().string(),
            .directory = filepath.parent_path().string()
        };

        std::string_view svSceneName = scene.name;
        mLoadedScenes[svSceneName] = std::move(scene);

        std::thread([this,
                     renderDevicePtr = &renderDevice,
                     filepath = std::move(filepath),
                     svSceneName,
                     directory = scene.directory] () {
            Assimp::Importer importer;
            importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, sRemoveComponents);
            importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, sRemovePrimitives);
            importer.SetPropertyBool(AI_CONFIG_PP_PTV_NORMALIZE, true);

            const aiScene* assimpScene = importer.ReadFile(filepath.string(), sImportFlags);

            if (!assimpScene || assimpScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !assimpScene->mRootNode)
                check(false, std::format("Failed to load model {}.", filepath.string()).c_str());

            // build scene
            mLoadedScenes[svSceneName].rootNode = buildScene(*assimpScene, *assimpScene->mRootNode, nullptr);

            // load meshes
            for (uint32_t i = 0; i < assimpScene->mNumMeshes; ++i)
            {
                const aiMesh& assimpMesh = *assimpScene->mMeshes[i];

                std::vector<::Mesh::Vertex> vertices = loadMeshVertices(assimpMesh);
                std::vector<uint32_t> indices = loadMeshIndices(assimpMesh);
                std::string meshName = assimpMesh.mName.C_Str();
                std::string materialName = assimpScene->mMaterials[assimpMesh.mMaterialIndex]->GetName().C_Str();

                enqueueTask([this,
                                renderDevicePtr,
                                vertices = std::move(vertices),
                                indices = std::move(indices),
                                meshName = std::move(meshName),
                                materialName = std::move(materialName),
                                svSceneName] () {
                    ::Mesh mesh;
                    mesh.create(*renderDevicePtr, vertices.size(), vertices.data(), indices.size(), indices.data(), meshName);
                    mLoadedScenes[svSceneName].meshes.push_back({.mesh = std::move(mesh), .materialName = std::move(materialName)});
                });
            }

            // load materials and textures
            for (uint32_t i = 0; i < assimpScene->mNumMaterials; ++i)
            {
                std::vector<std::future<LoadedTexture>> mLoadedTextures;

                const aiMaterial& material = *assimpScene->mMaterials[i];
                const std::string matName = material.GetName().C_Str();

                enqueueTask([this, svSceneName, matName] () {
                    mLoadedScenes.at(svSceneName).materials[matName].name = matName;
                });

                for (const auto& [textureName, textureType] : getMatTexNames(material))
                {
                    std::string texturePath = directory + textureName;
                    mLoadedTextures.push_back(std::async(std::launch::async, [texturePath, textureName, textureType, matName] () -> LoadedTexture {
                        int width, height, channels;
                        uint8_t* data = stbi_load(texturePath.c_str(), &width, &height, &channels, 4);
                        check(false, std::format("Failed to load {}", texturePath).c_str());

                        return {
                            .name = std::format("{}_{}", matName, textureName),
                            .type = textureType,
                            .width = static_cast<uint32_t >(width),
                            .height = static_cast<uint32_t >(height),
                            .bytes = data
                        };
                    }));
                }

                for (auto& future : mLoadedTextures)
                {
                    LoadedTexture loadedTexture = future.get();

                    enqueueTask([this,
                                 renderDevicePtr,
                                 loadedTexture = std::move(loadedTexture),
                                 matName,
                                 svSceneName] () {

                        Scene& scene = mLoadedScenes[svSceneName];
                        scene.materials[matName].mapNames[loadedTexture.type] = loadedTexture.name;

                        Texture texture {
                            .name = loadedTexture.name,
                            .texture = createTexture2D(*renderDevicePtr,
                                                       loadedTexture.width,
                                                       loadedTexture.height,
                                                       VK_FORMAT_R8G8B8A8_UNORM,
                                                       VK_IMAGE_USAGE_SAMPLED_BIT |
                                                           VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                                           VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                                       VK_IMAGE_ASPECT_COLOR_BIT,
                                                       TextureWrap::ClampToEdge,
                                                       TextureFilter::Anisotropic,
                                                       true)
                        };

                        transitionImageLayout(*renderDevicePtr,
                                              texture.texture.image,
                                              VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

                        uploadTextureData(*renderDevicePtr, texture.texture, loadedTexture.bytes);

                        transitionImageLayout(*renderDevicePtr,
                                              texture.texture.image,
                                              VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

                        generateMipMaps(*renderDevicePtr, texture.texture.image);

                        transitionImageLayout(*renderDevicePtr,
                                              texture.texture.image,
                                              VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                        stbi_image_free(loadedTexture.bytes);

                        scene.textures[loadedTexture.name] = std::move(texture);
                    });
                }
            }
        }).detach();
    }

    std::vector<std::pair<std::string, aiTextureType>> getMatTexNames(const aiMaterial& mat)
    {
        std::vector<std::pair<std::string, aiTextureType>> names;

        names.emplace_back(getTextureName(mat, aiTextureType_DIFFUSE), aiTextureType_DIFFUSE);
        names.emplace_back(getTextureName(mat, aiTextureType_SHININESS), aiTextureType_SHININESS);
        names.emplace_back(getTextureName(mat, aiTextureType_METALNESS ), aiTextureType_METALNESS );
        names.emplace_back(getTextureName(mat, aiTextureType_NORMALS), aiTextureType_NORMALS);
        names.emplace_back(getTextureName(mat, aiTextureType_HEIGHT), aiTextureType_HEIGHT);
        names.emplace_back(getTextureName(mat, aiTextureType_AMBIENT_OCCLUSION ), aiTextureType_AMBIENT_OCCLUSION );
        names.emplace_back(getTextureName(mat, aiTextureType_EMISSIVE), aiTextureType_EMISSIVE);
        names.emplace_back(getTextureName(mat, aiTextureType_SPECULAR), aiTextureType_SPECULAR);

        return names;
    }

    std::string getTextureName(const aiMaterial& material, aiTextureType type)
    {
        aiString aiPath;

        if (material.GetTextureCount(type))
        {
            material.GetTexture(type, 0, &aiPath);
            return aiPath.C_Str();
        }

        return {};
    }

    SceneNode buildScene(const aiScene& assimpScene, const aiNode& assimpNode, SceneNode* parent)
    {
        SceneNode sceneNode {
            .localTransform = assimpToGlmMat4(assimpNode.mTransformation)
        };

        for (uint32_t i = 0; i < assimpNode.mNumMeshes; ++i)
        {
            const aiMesh& mesh = *assimpScene.mMeshes[assimpNode.mMeshes[i]];
            sceneNode.meshes.push_back(mesh.mName.C_Str());
        }

        for (uint32_t i = 0; i < assimpNode.mNumChildren; ++i)
            sceneNode.children.push_back(buildScene(assimpScene, assimpNode, &sceneNode));

        return sceneNode;
    }
}
