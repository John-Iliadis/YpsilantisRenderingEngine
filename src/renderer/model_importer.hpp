//
// Created by Gianni on 23/01/2025.
//

#ifndef VULKANRENDERINGENGINE_MODEL_IMPORTER_HPP
#define VULKANRENDERINGENGINE_MODEL_IMPORTER_HPP

#include "tiny_gltf/tiny_gltf.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
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

    std::shared_ptr<tinygltf::Model> loadGltfScene(const std::filesystem::path& path);

    SceneNode createRootSceneNode(const tinygltf::Model& gltfModel, const tinygltf::Scene& gltfScene);
    SceneNode createRootSceneNode(const tinygltf::Model& gltfModel, const tinygltf::Node& gltfNode);

    glm::mat4 getNodeTransformation(const tinygltf::Node& gltfNode);

    Mesh createMesh(const VulkanRenderDevice* renderDevice, const MeshData& meshData);

    std::future<MeshData> createMeshData(const tinygltf::Model& model, const tinygltf::Mesh& gltfMesh);

    std::vector<Vertex> loadMeshVertices(const tinygltf::Model& model, const tinygltf::Mesh& mesh);

    const float* getBufferVertexData(const tinygltf::Model& model, const tinygltf::Primitive& primitive, const std::string& attribute);

    uint32_t getVertexCount(const tinygltf::Model& model, const tinygltf::Primitive& primitive);

    std::vector<uint32_t> loadMeshIndices(const tinygltf::Model& model, const tinygltf::Mesh& mesh);

    MaterialData loadMaterials(const tinygltf::Model& gltfModel);

    std::future<std::shared_ptr<ImageData>> loadImageData(const tinygltf::Model& gltfModel, const tinygltf::Texture& texture, const std::filesystem::path& directory);

    TextureWrap getWrapMode(int wrapMode);
    TextureMagFilter getMagFilter(int filterMode);
    TextureMinFilter getMipFilter(int filterMode);

    Texture createTexture(const VulkanRenderDevice* renderDevice, const std::shared_ptr<ImageData> imageData);
}

#endif //VULKANRENDERINGENGINE_MODEL_IMPORTER_HPP
