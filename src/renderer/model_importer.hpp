//
// Created by Gianni on 23/01/2025.
//

#ifndef VULKANRENDERINGENGINE_MODEL_IMPORTER_HPP
#define VULKANRENDERINGENGINE_MODEL_IMPORTER_HPP

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/GltfMaterial.h>
#include <assimp/texture.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include "../utils/loaded_image.hpp"
#include "../utils/utils.hpp"
#include "../utils/timer.hpp"
#include "vertex.hpp"
#include "model.hpp"

struct ModelImportData
{
    std::string path;
    bool normalize;
    bool flipUVs;
};

struct MeshData
{
    std::string name;
    VulkanBuffer vertexStagingBuffer;
    VulkanBuffer indexStagingBuffer;
    uint32_t materialIndex;
    bool opaque;
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
    ModelLoader(const VulkanRenderDevice& renderDevice, const ModelImportData& importData);
    ~ModelLoader() = default;

    ModelLoader(const ModelLoader& other) = delete;
    ModelLoader& operator=(const ModelLoader& other) = delete;

    ModelLoader(ModelLoader&& other) noexcept = delete;
    ModelLoader& operator=(ModelLoader&& other) noexcept = delete;

    bool success() const;

private:
    void createHierarchy(const aiScene& aiScene);
    void loadMeshes(const aiScene& aiScene);
    void loadTextures(const aiScene& aiScene);
    void loadMaterials(const aiScene& aiScene);
    void getTextureNames(const aiScene& aiScene);

    SceneNode createRootSceneNode(const aiScene& aiScene, const aiNode& aiNode);

    VulkanBuffer loadVertexData(const aiMesh& aiMesh);
    VulkanBuffer loadIndexData(const aiMesh& aiMesh);

    std::optional<ImageData> loadImageData(const std::string& texName);
    std::optional<ImageData> loadEmbeddedImageData(const aiScene& aiScene, const std::string& texName);

    std::optional<std::string> getTextureName(const aiMaterial& aiMaterial, aiTextureType type);

    Material loadMaterial(const aiMaterial& aiMaterial);
    glm::vec4 getBaseColorFactor(const aiMaterial& aiMaterial);
    glm::vec4 getEmissionFactor(const aiMaterial& aiMaterial);
    float getMetallicFactor(const aiMaterial& aiMaterial);
    float getRoughnessFactor(const aiMaterial& aiMaterial);
    float getAlphaMask(const aiMaterial& aiMaterial);
    float getAlphaCutoff(const aiMaterial& aiMaterial);
    bool isOpaque(const aiMaterial& aiMaterial);

    glm::mat4 assimpToGlmMat4(const aiMatrix4x4 &mat);

private:
    const VulkanRenderDevice& mRenderDevice;
    std::unordered_set<std::string> mTextureNames;
    std::unordered_map<std::string, int32_t> mInsertedTexIndex;
    bool mSuccess;
};

class ModelImporter : public SubscriberSNS
{
public:
    ModelImporter(const VulkanRenderDevice& renderDevice);
    ~ModelImporter() = default;

    void update();
    void notify(const Message &message) override;

private:
    void addModel(std::unique_ptr<ModelLoader>& modelData);
    void addMeshes(VkCommandBuffer commandBuffer, Model& model, const ModelLoader& modelData);
    void addTextures(VkCommandBuffer commandBuffer, Model& model, const ModelLoader& modelData);

public:
    const VulkanRenderDevice& mRenderDevice;

    std::unordered_map<std::filesystem::path, std::future<std::unique_ptr<ModelLoader>>> mModelDataFutures;
    std::unordered_map<std::filesystem::path, std::unique_ptr<ModelLoader>> mModelData;
    std::unordered_map<std::filesystem::path, std::shared_ptr<Model>> mModels;
    std::unordered_map<std::filesystem::path, VkFence> mFences;
};

#endif //VULKANRENDERINGENGINE_MODEL_IMPORTER_HPP
