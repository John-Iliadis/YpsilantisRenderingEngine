//
// Created by Gianni on 23/01/2025.
//

#include "resource_importer.hpp"

// todo: handle texture sampler and wrap
// todo: handle multiple scenes
// todo: handle multiple primitive types
// todo: handle embedded image data
// todo: error handling
// todo: handle texture loading fails
// todo: handle "Load error: No LoadImageData callback specified"

namespace ResourceImporter
{
    std::future<std::shared_ptr<LoadedModelData>> loadModel(const std::filesystem::path& path, const std::shared_ptr<VulkanRenderDevice> renderDevice, EnqueueCallback callback)
    {
        return std::async(std::launch::async, [path, renderDevice, callback] () -> std::shared_ptr<LoadedModelData> {
            std::shared_ptr<tinygltf::Model> gltfModel = loadGltfScene(path);
            std::shared_ptr<LoadedModelData> modelData = std::make_shared<LoadedModelData>();

            modelData->path = path;
            modelData->name = path.filename().string();
            modelData->root = createModelGraph(*gltfModel, gltfModel->nodes.at(gltfModel->scenes.at(0).nodes.at(0)));
            modelData->materials = loadMaterials(*gltfModel);
            modelData->indirectTextureMap = createIndirectTextureToImageMap(*gltfModel);
            modelData->bb = computeBoundingBox(*gltfModel, 0, glm::identity<glm::mat4>());

            // load mesh data
            std::vector<std::future<MeshData>> meshDataFutures;
            for (const auto& gltfMesh : gltfModel->meshes)
                meshDataFutures.push_back(createMeshData(*gltfModel, gltfMesh));

            // upload mesh data to opengl
            for (auto& meshDataFuture : meshDataFutures)
            {
                callback([renderDevice, modelData, meshData = meshDataFuture.get()] () {
                    modelData->meshes.push_back(createMesh(renderDevice, meshData));
                });
            }

            // load texture data
            std::filesystem::path directory = path.parent_path();
            std::vector<std::future<std::shared_ptr<ImageLoader>>> loadedImageFutures;
            for (const auto& image : gltfModel->images)
                loadedImageFutures.push_back(loadImageData(image, directory));

            // upload texture data to opengl
            for (auto& loadedImageFuture : loadedImageFutures)
            {
                callback([renderDevice, modelData, imageData = loadedImageFuture.get()] () {
                    modelData->textures.push_back(makeTexturePathPair(renderDevice, imageData));
                });
            }

            return modelData;
        });
    }

    std::shared_ptr<tinygltf::Model> loadGltfScene(const std::filesystem::path &path)
    {
        debugLog("ResourceImporter: Loading " + path.string());

        std::shared_ptr<tinygltf::Model> model = std::make_shared<tinygltf::Model>();
        tinygltf::TinyGLTF loader;
        std::string error;
        std::string warning;

        if (fileExtension(path) == ".gltf")
            loader.LoadASCIIFromFile(model.get(), &error, &warning, path.string());
        else if (fileExtension(path) == ".glb")
            loader.LoadBinaryFromFile(model.get(), &error, &warning, path.string());

        check(error.empty(), std::format("Failed to load model {}\nLoad error: {}", path.string(), error).c_str());

        if (!warning.empty())
            debugLog("ResourceImporter::loadGltfScene: Warning: " + warning);

        return model;
    }

    LoadedModelData::Node createModelGraph(const tinygltf::Model& model, const tinygltf::Node& gltfNode)
    {

        LoadedModelData::Node node {
            .name = gltfNode.name,
            .transformation = getNodeTransformation(gltfNode)
        };

        if (gltfNode.mesh != -1)
            node.meshIndex = gltfNode.mesh;

        for (size_t i = 0; i < gltfNode.children.size(); ++i)
            node.children.push_back(createModelGraph(model, model.nodes.at(gltfNode.children.at(i))));

        return node;
    }

    glm::mat4 getNodeTransformation(const tinygltf::Node& node)
    {
        glm::mat4 transformation = glm::identity<glm::mat4>();

        if (!node.matrix.empty())
        {
            transformation = glm::make_mat4(node.matrix.data());
        }
        else
        {
            if (!node.translation.empty())
            {
                transformation = glm::translate(transformation, glm::make_vec3((float*)node.translation.data()));
            }

            if (!node.rotation.empty())
            {
                glm::quat q = glm::make_quat(node.rotation.data());
                transformation *= glm::mat4(q);
            }

            if (!node.scale.empty())
            {
                transformation = glm::scale(transformation, glm::make_vec3((float*)node.scale.data()));
            }
        }

        return transformation;
    }

    LoadedModelData::Mesh createMesh(const std::shared_ptr<VulkanRenderDevice> renderDevice, const MeshData &meshData)
    {
        return {
            .name = meshData.name,
            .mesh = std::make_shared<InstancedMesh>(*renderDevice, meshData.vertices, meshData.indices),
            .materialIndex = meshData.materialIndex
        };
    }

    std::future<MeshData> createMeshData(const tinygltf::Model& model, const tinygltf::Mesh& gltfMesh)
    {
        debugLog(std::format("ResourceImporter: Loading mesh {}", gltfMesh.name));
        return std::async(std::launch::async, [&model, &gltfMesh]() -> MeshData
        {
            MeshData meshData {
                .name = gltfMesh.name,
                .vertices = loadMeshVertices(model, gltfMesh),
                .indices = loadMeshIndices(model, gltfMesh)
            };

            if (gltfMesh.primitives.at(0).material != -1)
                meshData.materialIndex = gltfMesh.primitives.at(0).material;

            return meshData;
        });
    }

    std::vector<Vertex> loadMeshVertices(const tinygltf::Model& model, const tinygltf::Mesh& mesh)
    {
        std::vector<Vertex> vertices;

        const tinygltf::Primitive& primitive = mesh.primitives.at(0);

        size_t vertexCount = getVertexCount(model, primitive);
        vertices.reserve(vertexCount);

        const float* positionBuffer = getBufferVertexData(model, primitive, "POSITION");
        const float* texCoordsBuffer = getBufferVertexData(model, primitive, "TEXCOORD_0");
        const float* normalsBuffer = getBufferVertexData(model, primitive, "NORMAL");
        const float* tangentBuffer = getBufferVertexData(model, primitive, "TANGENT");

        for (size_t i = 0; i < vertexCount; ++i)
        {
            Vertex vertex{};

            if (positionBuffer)
            {
                vertex.position = glm::make_vec3(&positionBuffer[i * 3]);
            }

            if (texCoordsBuffer)
            {
                vertex.texCoords = glm::make_vec2(&texCoordsBuffer[i * 2]);
            }

            if (normalsBuffer)
            {
                vertex.normal = glm::normalize(glm::make_vec3(&normalsBuffer[i * 3]));
            }

            if (tangentBuffer)
            {
                vertex.tangent = glm::normalize(glm::make_vec3(&tangentBuffer[i * 3]));
                vertex.bitangent = glm::normalize(glm::cross(vertex.tangent, vertex.normal));
            }

            vertices.push_back(vertex);
        }

        return vertices;
    }

    const float* getBufferVertexData(const tinygltf::Model& model, const tinygltf::Primitive& primitive, const std::string& attribute)
    {
        const float* data = nullptr;

        if (primitive.attributes.contains(attribute))
        {
            const tinygltf::Accessor& accessor = model.accessors.at(primitive.attributes.at(attribute));
            const tinygltf::BufferView& bufferView = model.bufferViews.at(accessor.bufferView);
            const tinygltf::Buffer& buffer = model.buffers.at(bufferView.buffer);
            data = reinterpret_cast<const float*>(&buffer.data.at(bufferView.byteOffset + accessor.byteOffset));
        }

        return data;
    }

    uint32_t getVertexCount(const tinygltf::Model& model, const tinygltf::Primitive& primitive)
    {
        if (primitive.attributes.contains("POSITION"))
            return model.accessors.at(primitive.attributes.at("POSITION")).count;
        return 0;
    }

    std::vector<uint32_t> loadMeshIndices(const tinygltf::Model& model, const tinygltf::Mesh& mesh)
    {
        std::vector<uint32_t> indices;

        const tinygltf::Primitive& primitive = mesh.primitives.at(0);

        if (primitive.indices == -1)
            return indices;

        indices.reserve(model.accessors.at(primitive.indices).count);

        const tinygltf::Accessor& accessor = model.accessors.at(primitive.indices);
        const tinygltf::BufferView& bufferView = model.bufferViews.at(accessor.bufferView);
        const tinygltf::Buffer& buffer = model.buffers.at(bufferView.buffer);
        const uint8_t* data = &buffer.data.at(bufferView.byteOffset + accessor.byteOffset);

        for (size_t i = 0; i < accessor.count; ++i)
            indices.push_back(static_cast<uint32_t>(data[i]));

        return indices;
    }

    std::vector<LoadedModelData::Material> loadMaterials(const tinygltf::Model& model)
    {
        std::vector<LoadedModelData::Material> materials(model.materials.size());
        for (size_t i = 0; i < model.materials.size(); ++i)
        {
            const tinygltf::Material& gltfMaterial = model.materials.at(i);

            materials.at(i).name = gltfMaterial.name;

            int32_t baseColorTexIndex = gltfMaterial.pbrMetallicRoughness.baseColorTexture.index;
            int32_t metallicRoughnessTexIndex = gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index;
            int32_t normalTexIndex = gltfMaterial.normalTexture.index;
            int32_t aoTexIndex = gltfMaterial.occlusionTexture.index;
            int32_t emissionTexIndex = gltfMaterial.emissiveTexture.index;

            if (baseColorTexIndex != -1)
            {
                materials.at(i).baseColorTexIndex = model.textures.at(baseColorTexIndex).source;
            }

            if (metallicRoughnessTexIndex != -1)
            {
                materials.at(i).metallicRoughnessTexIndex = model.textures.at(metallicRoughnessTexIndex).source;
            }

            if (normalTexIndex != -1)
            {
                materials.at(i).normalTexIndex = model.textures.at(normalTexIndex).source;
            }

            if (aoTexIndex != -1)
            {
                materials.at(i).aoTexIndex = model.textures.at(aoTexIndex).source;
            }

            if (emissionTexIndex != -1)
            {
                materials.at(i).emissionTexIndex = model.textures.at(emissionTexIndex).source;
            }

            materials.at(i).baseColorFactor = glm::make_vec4(gltfMaterial.pbrMetallicRoughness.baseColorFactor.data());
            materials.at(i).emissionColorFactor = glm::vec4(glm::make_vec3(gltfMaterial.emissiveFactor.data()), 0.f);

            materials.at(i).metallicFactor = static_cast<float>(gltfMaterial.pbrMetallicRoughness.metallicFactor);
            materials.at(i).roughnessFactor = static_cast<float>(gltfMaterial.pbrMetallicRoughness.roughnessFactor);
            materials.at(i).occlusionFactor = static_cast<float>(gltfMaterial.occlusionTexture.strength);
        }

        return materials;
    }

    std::unordered_map<int32_t, uint32_t> createIndirectTextureToImageMap(const tinygltf::Model& model)
    {
        std::unordered_map<int32_t, uint32_t> map;

        for (size_t i = 0; i < model.textures.size(); ++i)
            map.emplace(i, model.textures.at(i).source);

        return map;
    }

    std::future<std::shared_ptr<ImageLoader>> loadImageData(const tinygltf::Image& image, const std::filesystem::path& directory)
    {
        debugLog(std::format("ResourceImporter: Loading image {}", (directory / image.uri).string()));
        return std::async(std::launch::async, [&image, &directory] () -> std::shared_ptr<ImageLoader> {
            return std::make_shared<ImageLoader>(directory / image.uri);
        });
    }

    std::pair<std::shared_ptr<VulkanTexture>, std::filesystem::path> makeTexturePathPair(const std::shared_ptr<VulkanRenderDevice> renderDevice, const std::shared_ptr<ImageLoader>& imageData)
    {
        TextureSpecification textureSpecification {
            .format = imageData->format(),
            .width = static_cast<uint32_t>(imageData->width()),
            .height = static_cast<uint32_t>(imageData->height()),
            .layerCount = 1,
            .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
            .imageUsage =
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                VK_IMAGE_USAGE_SAMPLED_BIT,
            .imageAspect = VK_IMAGE_ASPECT_COLOR_BIT,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .wrapMode = TextureWrap::ClampToEdge,
            .filterMode = TextureFilter::Anisotropic,
            .generateMipMaps = true
        };

        auto vulkanTexture = std::make_shared<VulkanTexture>(*renderDevice, textureSpecification, imageData->data());
        vulkanTexture->vulkanImage.transitionLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        vulkanTexture->generateMipMaps();
        vulkanTexture->vulkanImage.transitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        vulkanTexture->setDebugName(imageData->path().string());

        return {vulkanTexture, imageData->path()};
    }

    BoundingBox computeBoundingBox(const tinygltf::Model& model, int nodeIndex, const glm::mat4& parentTransform)
    {
        BoundingBox bb;

        const tinygltf::Node &node = model.nodes[nodeIndex];

        glm::mat4 nodeTransform = parentTransform * getNodeTransformation(node);

        if (node.mesh != -1)
        {
            const tinygltf::Mesh &mesh = model.meshes[node.mesh];

            for (const auto &primitive : mesh.primitives)
            {
                const float *positions = getBufferVertexData(model, primitive, "POSITION");

                if (!positions)
                    continue;

                size_t vertexCount = getVertexCount(model, primitive);
                for (size_t i = 0; i < vertexCount; i++)
                {
                    glm::vec4 vertex = glm::vec4(glm::make_vec3(&positions[i * 3]), 1.f);

                    glm::vec3 transformedVertex = glm::vec3(nodeTransform * vertex);

                    bb.expand(transformedVertex);
                }
            }
        }

        for (int child : node.children)
        {
            BoundingBox childBb = computeBoundingBox(model, child, nodeTransform);
            bb.min = glm::min(bb.min, childBb.min);
            bb.max = glm::max(bb.max, childBb.max);
        }

        return bb;
    }
}
