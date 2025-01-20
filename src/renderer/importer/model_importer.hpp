//
// Created by Gianni on 19/01/2025.
//

#ifndef VULKANRENDERINGENGINE_MODEL_IMPORTER_HPP
#define VULKANRENDERINGENGINE_MODEL_IMPORTER_HPP

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include "loaded_resource.hpp"

using Task = std::function<void()>;
namespace fs = std::filesystem;

// todo: error handling
class ModelImporter
{
public:
    ModelImporter();

    void create(const VulkanRenderDevice& renderDevice);
    void importModel(const std::string& path);
    void processMainThreadTasks();

    void setImportFinishedCallback(std::function<void(LoadedModel&&)> callback);

private:
    void enqueue(Task task);

    // main thread tasks
    LoadedModel::Mesh createMesh(const LoadedMesh& loadedMesh);
    std::pair<std::string, NamedTexture> createNamedTexturePair(const LoadedTexture<uint8_t>& loadedTexture);

    // non main thread tasks
    std::future<LoadedModel> loadModel(const fs::path& path);
    static std::shared_ptr<aiScene> loadAssimpScene(const fs::path& path);
    static ModelNode createModelGraph(const aiNode* assimpNode);
    static std::future<LoadedMesh> createLoadedMesh(const aiMesh& mesh);
    static std::future<std::optional<LoadedTexture<uint8_t>>> createLoadedTexture(const std::string& path);
    static LoadedModel::Material createMaterial(const aiMaterial& assimpMaterial, const std::string& directory);

    static std::vector<InstancedMesh::Vertex> loadMeshVertices(const aiMesh& mesh);
    static std::vector<uint32_t> loadMeshIndices(const aiMesh& mesh);
    static glm::mat4 assimpToGlmMat4(const aiMatrix4x4& mat);
    static TextureType getTextureType(aiTextureType type);
    static aiTextureType getTextureType(TextureType type);
    static std::string getTextureName(const aiMaterial& material, TextureType type);

private:
    const VulkanRenderDevice* mRenderDevice;

    std::vector<Task> mMainThreadTasks;
    std::mutex mMainThreadTasksMutex;

    std::vector<std::future<LoadedModel>> mLoadedModelFutures;
    std::function<void(LoadedModel&&)> mOnModelImportFinished;
};

#endif //VULKANRENDERINGENGINE_MODEL_IMPORTER_HPP
