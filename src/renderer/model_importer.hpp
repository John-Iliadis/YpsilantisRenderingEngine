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
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    uint32_t materialIndex;
    glm::vec3 center;
};

struct ImageData
{
    int32_t width;
    int32_t height;
    std::string name;
    std::unique_ptr<uint8_t, std::function<void(uint8_t*)>> imageData;
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
    ModelLoader() = default;
    ModelLoader(const ModelImportData& importData);
    ~ModelLoader() = default;

    ModelLoader(const ModelLoader& other) = delete;
    ModelLoader& operator=(const ModelLoader& other) = delete;

    ModelLoader(ModelLoader&& other) noexcept;
    ModelLoader& operator=(ModelLoader&& other) noexcept;

    void swap(ModelLoader& other) noexcept;
    bool success() const;

private:
    void createHierarchy(const aiScene& aiScene);
    void loadMeshes(const aiScene& aiScene);
    void loadTextures(const aiScene& aiScene);
    void loadMaterials(const aiScene& aiScene);
    void getTextureNames(const aiScene& aiScene);

    SceneNode createRootSceneNode(const aiScene& aiScene, const aiNode& aiNode);

    std::vector<Vertex> loadMeshVertices(const aiMesh& aiMesh);
    std::vector<uint32_t> loadMeshIndices(const aiMesh& aiMesh);

    std::optional<ImageData> loadImageData(const std::string& texName);
    std::optional<ImageData> loadEmbeddedImageData(const aiScene& aiScene, const std::string& texName);

    std::optional<std::string> getTextureName(const aiMaterial& aiMaterial, aiTextureType type);

    Material loadMaterial(const aiMaterial& aiMaterial);
    glm::vec4 getBaseColorFactor(const aiMaterial& aiMaterial);
    glm::vec4 getEmissionFactor(const aiMaterial& aiMaterial);
    float getMetallicFactor(const aiMaterial& aiMaterial);
    float getRoughnessFactor(const aiMaterial& aiMaterial);
    float getAlphaCutoff(const aiMaterial& aiMaterial);
    AlphaMode getAlphaMode(const aiMaterial& aiMaterial);

    glm::mat4 assimpToGlmMat4(const aiMatrix4x4 &mat);

private:
    std::unordered_set<std::string> mTextureNames;
    std::unordered_map<std::string, int32_t> mInsertedTexIndex;
    bool mSuccess {};
};

#endif //VULKANRENDERINGENGINE_MODEL_IMPORTER_HPP
