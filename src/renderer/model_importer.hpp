//
// Created by Gianni on 23/01/2025.
//

#ifndef VULKANRENDERINGENGINE_MODEL_IMPORTER_HPP
#define VULKANRENDERINGENGINE_MODEL_IMPORTER_HPP

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/material.h>
#include <assimp/texture.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../utils/loaded_image.hpp"
#include "../utils/utils.hpp"
#include "../utils/timer.hpp"
#include "vertex.hpp"
#include "model.hpp"

using EnqueueCallback = std::function<void(std::function<void()>&&)>;

struct MeshData
{
    std::string name;
    VulkanBuffer vertexStagingBuffer;
    VulkanBuffer indexStagingBuffer;
    uint32_t materialIndex;
};

struct ImageData
{
    int32_t width;
    int32_t height;
    std::string name;
    VulkanBuffer stagingBuffer;
    TextureMagFilter magFilter;
    TextureMinFilter minFilter;
    TextureWrap wrapModeS;
    TextureWrap wrapModeT;
};

struct MaterialData
{
    std::vector<Material> materials;
    std::vector<std::string> materialNames;
};

class ModelLoader
{
public:
    std::filesystem::path path;
    SceneNode root;
    std::vector<MeshData> meshes;
    std::vector<ImageData> images;
    std::vector<Material> materials;
    std::vector<std::string> materialNames;

public:
    ModelLoader(const VulkanRenderDevice& renderDevice, const std::filesystem::path& path);

    ModelLoader(const ModelLoader& other) = delete;
    ModelLoader& operator=(const ModelLoader& other) = delete;

    ModelLoader(ModelLoader&& other) noexcept = delete;
    ModelLoader& operator=(ModelLoader&& other) noexcept = delete;

    bool success() const;

private:
    void createHierarchy();
    void loadMeshes();
    void loadTextures();
    void loadMaterials();
    void getTextureNames();

    SceneNode createRootSceneNode(const aiNode& aiNode);

    VulkanBuffer loadVertexData(const aiMesh& aiMesh);
    VulkanBuffer loadIndexData(const aiMesh& aiMesh);

    std::optional<ImageData> loadImageData(const std::string& texName);
    std::optional<ImageData> loadEmbeddedImageData(const std::string& texName);

    std::optional<std::string> getTextureName(const aiMaterial& aiMaterial, aiTextureType type);

    Material loadMaterial(const aiMaterial& aiMaterial);
    glm::vec4 getBaseColorFactor(const aiMaterial& aiMaterial);
    glm::vec4 getEmissionFactor(const aiMaterial& aiMaterial);
    float getMetallicFactor(const aiMaterial& aiMaterial);
    float getRoughnessFactor(const aiMaterial& aiMaterial);

    glm::mat4 assimpToGlmMat4(const aiMatrix4x4 &mat);

private:
    const VulkanRenderDevice& mRenderDevice;
    Assimp::Importer mImporter;
    std::unordered_set<std::string> mTextureNames;
    std::unordered_map<std::string, int32_t> mInsertedTexIndex;
    bool mSuccess;
};


#endif //VULKANRENDERINGENGINE_MODEL_IMPORTER_HPP
