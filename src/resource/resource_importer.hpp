//
// Created by Gianni on 23/01/2025.
//

#ifndef VULKANRENDERINGENGINE_RESOURCE_IMPORTER_HPP
#define VULKANRENDERINGENGINE_RESOURCE_IMPORTER_HPP

#include <tiny_gltf/tiny_gltf.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "../utils/image_loader.hpp"
#include "../utils/utils.hpp"
#include "loaded_resource.hpp"

using EnqueueCallback = std::function<void(std::function<void()>&&)>;

namespace ResourceImporter
{
    std::future<std::shared_ptr<LoadedModelData>> loadModel(const std::filesystem::path& path, const std::shared_ptr<VulkanRenderDevice> renderDevice, EnqueueCallback callback);

    std::shared_ptr<tinygltf::Model> loadGltfScene(const std::filesystem::path& path);

    LoadedModelData::Node createModelGraph(const tinygltf::Model& model, const tinygltf::Node& gltfNode);

    glm::mat4 getNodeTransformation(const tinygltf::Node& node);

    LoadedModelData::Mesh createMesh(const std::shared_ptr<VulkanRenderDevice> renderDevice, const MeshData& meshData);

    std::future<MeshData> createMeshData(const tinygltf::Model& model, const tinygltf::Mesh& gltfMesh);

    std::vector<Vertex> loadMeshVertices(const tinygltf::Model& model, const tinygltf::Mesh& mesh);

    const float* getBufferVertexData(const tinygltf::Model& model, const tinygltf::Primitive& primitive, const std::string& attribute);

    uint32_t getVertexCount(const tinygltf::Model& model, const tinygltf::Primitive& primitive);

    std::vector<uint32_t> loadMeshIndices(const tinygltf::Model& model, const tinygltf::Mesh& mesh);

    std::vector<LoadedModelData::Material> loadMaterials(const tinygltf::Model& model);

    std::unordered_map<int32_t, uint32_t> createIndirectTextureToImageMap(const tinygltf::Model& model);

    std::future<std::shared_ptr<ImageLoader>> loadImageData(const tinygltf::Image& image, const std::filesystem::path& directory);

    std::pair<std::shared_ptr<VulkanTexture>, std::filesystem::path> makeTexturePathPair(const std::shared_ptr<VulkanRenderDevice> renderDevice, const std::shared_ptr<ImageLoader>& imageData);

    BoundingBox computeBoundingBox(const tinygltf::Model& model, int nodeIndex, const glm::mat4& parentTransform);
}

#endif //VULKANRENDERINGENGINE_RESOURCE_IMPORTER_HPP
