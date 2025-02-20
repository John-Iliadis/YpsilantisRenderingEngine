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
#include "vertex.hpp"
#include "model.hpp"

using EnqueueCallback = std::function<void(std::function<void()>&&)>;

struct MeshData
{
    std::string name;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    int32_t materialIndex;
};

struct ImageData
{
    LoadedImage loadedImage;
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

namespace ModelImporter
{
    std::future<std::shared_ptr<Model>> loadModel(const std::filesystem::path& path, const VulkanRenderDevice* renderDevice, EnqueueCallback callback);

    std::shared_ptr<aiScene> loadScene(const std::filesystem::path& path);

    SceneNode createRootSceneNode(const aiNode& assimpNode);

    Mesh createMesh(const VulkanRenderDevice* renderDevice, const MeshData& meshData);

    std::future<MeshData> loadMeshData(const aiMesh& assimpMesh);

    std::vector<Vertex> loadMeshVertices(const aiMesh& assimpMesh);

    std::vector<uint32_t> loadMeshIndices(const aiMesh& assimpMesh);

    std::future<std::shared_ptr<ImageData>> loadImageData(const std::filesystem::path& directory, const std::string& filename);

    Texture createTexture(const VulkanRenderDevice* renderDevice, std::shared_ptr<ImageData> imageData);

    MaterialData loadMaterials(const aiScene& assimpScene, const std::unordered_map<std::string, int32_t>& texNames);
    Material loadMaterial(const aiMaterial& assimpMaterial, const std::unordered_map<std::string, int32_t>& texNames);
    glm::vec4 getBaseColorFactor(const aiMaterial& assimpMaterial);
    glm::vec4 getEmissionFactor(const aiMaterial& assimpMaterial);
    float getMetallicFactor(const aiMaterial& assimpMaterial);
    float getRoughnessFactor(const aiMaterial& assimpMaterial);

    std::unordered_map<std::string, int32_t> getTextureNames(const aiScene& assimpScene);
    std::optional<std::string> getTextureName(const aiMaterial& material, aiTextureType type);

    glm::mat4 assimpToGlmMat4(const aiMatrix4x4 &mat);
}

#endif //VULKANRENDERINGENGINE_MODEL_IMPORTER_HPP
