//
// Created by Gianni on 13/01/2025.
//

#ifndef VULKANRENDERINGENGINE_IMPORTER_HPP
#define VULKANRENDERINGENGINE_IMPORTER_HPP

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "scene.hpp"
#include "../mesh.hpp"

namespace Importer
{
    using Task = std::function<void()>;

    struct LoadedTexture
    {
        std::string name;
        aiTextureType type;
        uint32_t width;
        uint32_t height;
        uint8_t* bytes; // don't forget to call free
    };

    class ModelImporter
    {
    public:
        // todo: set texture & mesh debug name
        void importModel(const std::filesystem::path& filepath, const VulkanRenderDevice& renderDevice);

        void processMainThread();

    private:
        void enqueueTask(Task task);

    private:
        std::unordered_map<std::string_view, Scene> mLoadedScenes;
        std::unordered_map<std::string_view, std::atomic<bool>> mLoadComplete;
        std::vector<Task> mTaskQueue;
        std::mutex mTaskQueueMutex;
    };

    std::vector<::Mesh::Vertex> loadMeshVertices(const aiMesh& mesh);
    std::vector<uint32_t> loadMeshIndices(const aiMesh& mesh);
    glm::mat4 assimpToGlmMat4(const aiMatrix4x4& mat);
    std::vector<std::pair<std::string, aiTextureType>> getMatTexNames(const aiMaterial& mat);
    std::string getTextureName(const aiMaterial& material, aiTextureType type);
    SceneNode buildScene(const aiScene& assimpScene, const aiNode& assimpNode, SceneNode* parent);
}

#endif //VULKANRENDERINGENGINE_IMPORTER_HPP
