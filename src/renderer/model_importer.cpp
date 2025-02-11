//
// Created by Gianni on 23/01/2025.
//

#include "model_importer.hpp"

// todo: load all mesh primitives
// todo: handle multiple primitive types
// todo: handle embedded image data
// todo: error handling
// todo: handle texture loading fails
// todo: handle "Load error: No LoadImageData callback specified"
// todo: load lights

namespace ModelImporter
{
    std::future<std::shared_ptr<Model>> loadModel(const std::filesystem::path& path, const VulkanRenderDevice* renderDevice, EnqueueCallback callback)
    {
        return std::async(std::launch::async, [path, renderDevice, callback] () {
            std::shared_ptr<tinygltf::Model> gltfModel = loadGltfScene(path);
            std::shared_ptr<Model> model = std::make_shared<Model>();

            model->name = path.filename().string();
            model->path = path;
            model->scenes = loadScenes(*gltfModel);

            // load mesh data
            std::vector<std::future<MeshData>> meshDataFutures;
            for (const auto& gltfMesh : gltfModel->meshes)
                meshDataFutures.push_back(createMeshData(*gltfModel, gltfMesh));

            // upload mesh data to vulkan
            for (auto& meshDataFuture : meshDataFutures)
            {
                callback([renderDevice, model, meshData = meshDataFuture.get()] () {
                    model->meshes.push_back(createMesh(renderDevice, meshData));
                });
            }

            // load materials
            MaterialData materialData = loadMaterials(*gltfModel);

            model->materials = std::move(materialData.materials);
            model->materialNames = std::move(materialData.materialNames);

            // load texture data
            std::filesystem::path directory = path.parent_path();
            std::vector<std::future<std::shared_ptr<ImageData>>> loadedImageFutures;
            for (const auto& texture : gltfModel->textures)
                loadedImageFutures.push_back(loadImageData(*gltfModel, texture, directory));

            // upload texture data to vulkan
            for (auto& loadedImageFuture : loadedImageFutures)
            {
                callback([renderDevice, model, imageData = loadedImageFuture.get()] () {
                    model->textures.push_back(createTexture(renderDevice, imageData));
                });
            }

            return model;
        });
    }

    std::shared_ptr<tinygltf::Model> loadGltfScene(const std::filesystem::path &path)
    {
        debugLog("Loading model: " + path.string());

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
            debugLog("Model load warning: " + warning);

        return model;
    }

    std::vector<SceneNode> loadScenes(const tinygltf::Model& gltfModel)
    {
        std::vector<SceneNode> rootNodes;

        if (gltfModel.scenes.size() == 1)
        {
            const tinygltf::Scene& scene = gltfModel.scenes.at(0);
            const tinygltf::Node& root = gltfModel.nodes.at(scene.nodes.at(0));

            SceneNode rootSceneNode = createRootSceneNode(gltfModel, root);
            rootNodes.push_back(std::move(rootSceneNode));
        }
        else
        {
            for (const tinygltf::Scene& scene : gltfModel.scenes)
            {
                SceneNode rootSceneNode = createRootSceneNode(gltfModel, scene);
                rootNodes.push_back(std::move(rootSceneNode));
            }
        }

        return rootNodes;
    }

    SceneNode createRootSceneNode(const tinygltf::Model& gltfModel, const tinygltf::Scene& gltfScene)
    {
        SceneNode sceneNode {
            .name = gltfScene.name.empty()? "Unnamed" : gltfScene.name,
            .transformation = glm::identity<glm::mat4>(),
            .meshIndex = -1
        };

        for (int32_t nodeIndex : gltfScene.nodes)
        {
            const tinygltf::Node& gltfNode = gltfModel.nodes.at(nodeIndex);
            sceneNode.children.push_back(createRootSceneNode(gltfModel, gltfNode));
        }

        return sceneNode;
    }

    SceneNode createRootSceneNode(const tinygltf::Model& gltfModel, const tinygltf::Node& gltfNode)
    {
        SceneNode sceneNode {
            .name = gltfNode.name.empty()? "Unnamed" : gltfNode.name,
            .transformation = getNodeTransformation(gltfNode),
            .meshIndex = gltfNode.mesh
        };

        for (int32_t childIndex : gltfNode.children)
        {
            const tinygltf::Node& gltfChildNode = gltfModel.nodes.at(childIndex);
            sceneNode.children.push_back(createRootSceneNode(gltfModel, gltfChildNode));
        }

        return sceneNode;
    }

    glm::mat4 getNodeTransformation(const tinygltf::Node& gltfNode)
    {
        glm::mat4 transformation = glm::identity<glm::mat4>();

        if (!gltfNode.matrix.empty())
        {
            transformation = glm::make_mat4(gltfNode.matrix.data());
        }
        else
        {
            if (!gltfNode.translation.empty())
            {
                glm::dvec3 translation = glm::make_vec3(gltfNode.translation.data());
                transformation = glm::translate(transformation, static_cast<glm::vec3>(translation));
            }

            if (!gltfNode.rotation.empty())
            {
                glm::dquat q = glm::make_quat(gltfNode.rotation.data());
                transformation *= glm::mat4(static_cast<glm::quat>(q));
            }

            if (!gltfNode.scale.empty())
            {
                glm::dvec3 scale = glm::make_vec3(gltfNode.scale.data());
                transformation = glm::scale(transformation, static_cast<glm::vec3>(scale));
            }
        }

        return transformation;
    }

    Mesh createMesh(const VulkanRenderDevice* renderDevice, const MeshData &meshData)
    {
        Mesh mesh {
            .meshID = UUIDRegistry::generateMeshID(),
            .name = meshData.name.empty()? "Unnamed" : meshData.name,
            .mesh = InstancedMesh(*renderDevice, meshData.vertices, meshData.indices),
            .materialIndex = meshData.materialIndex
        };

        return mesh;
    }

    std::future<MeshData> createMeshData(const tinygltf::Model& model, const tinygltf::Mesh& gltfMesh)
    {
        debugLog("Loading mesh: " + gltfMesh.name);

        return std::async(std::launch::async, [&model, &gltfMesh] () {

            MeshData meshData {
                .name = gltfMesh.name,
                .vertices = loadMeshVertices(model, gltfMesh),
                .indices = loadMeshIndices(model, gltfMesh),
                .materialIndex = gltfMesh.primitives.at(0).material
            };

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

    MaterialData loadMaterials(const tinygltf::Model& gltfModel)
    {
        MaterialData materialData;

        for (const tinygltf::Material& gltfMaterial : gltfModel.materials)
        {
            Material material {
                .baseColorTexIndex = gltfMaterial.pbrMetallicRoughness.baseColorTexture.index,
                .metallicRoughnessTexIndex = gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index,
                .normalTexIndex = gltfMaterial.normalTexture.index,
                .aoTexIndex = gltfMaterial.occlusionTexture.index,
                .emissionTexIndex = gltfMaterial.emissiveTexture.index,
                .metallicFactor = static_cast<float>(gltfMaterial.pbrMetallicRoughness.metallicFactor),
                .roughnessFactor = static_cast<float>(gltfMaterial.pbrMetallicRoughness.roughnessFactor),
                .occlusionFactor = static_cast<float>(gltfMaterial.occlusionTexture.strength),
                .baseColorFactor = glm::make_vec4(gltfMaterial.pbrMetallicRoughness.baseColorFactor.data()),
                .emissionFactor = glm::vec4(glm::make_vec3(gltfMaterial.emissiveFactor.data()), 0.f),
                .tiling = glm::vec2(1.f),
                .offset = glm::vec2(0.f)
            };

            materialData.materials.push_back(material);
            materialData.materialNames.push_back(gltfMaterial.name);
        }

        return materialData;
    }

    // todo: handle load fail
    std::future<std::shared_ptr<ImageData>> loadImageData(const tinygltf::Model& gltfModel, const tinygltf::Texture& texture, const std::filesystem::path& directory)
    {
        const tinygltf::Image& image = gltfModel.images.at(texture.source);

        std::filesystem::path imagePath = directory / image.uri;

        debugLog("Loading image: " + imagePath.string());

        return std::async(std::launch::async, [&gltfModel, &texture, imagePath] () {
            std::shared_ptr<ImageData> imageData = std::make_shared<ImageData>();

            if (texture.sampler != -1)
            {
                const tinygltf::Sampler& sampler = gltfModel.samplers.at(texture.sampler);

                imageData->magFilter = getMagFilter(sampler.magFilter);
                imageData->minFilter = getMipFilter(sampler.minFilter);
                imageData->wrapModeS = getWrapMode(sampler.wrapS);
                imageData->wrapModeT = getWrapMode(sampler.wrapT);
            }
            else
            {
                imageData->magFilter = getMagFilter(-1);
                imageData->minFilter = getMipFilter(-1);
                imageData->wrapModeS = getWrapMode(-1);
                imageData->wrapModeT = getWrapMode(-1);
            }

            imageData->loadedImage = LoadedImage(imagePath);

            return imageData;
        });
    }

    TextureWrap getWrapMode(int wrapMode)
    {
        switch (wrapMode)
        {
            case TINYGLTF_TEXTURE_WRAP_REPEAT: return TextureWrap::Repeat;
            case TINYGLTF_TEXTURE_WRAP_CLAMP_TO_EDGE: return TextureWrap::ClampToEdge;
            case TINYGLTF_TEXTURE_WRAP_MIRRORED_REPEAT: return TextureWrap::MirroredRepeat;
        }

        return TextureWrap::Repeat;
    }

    TextureMagFilter getMagFilter(int filterMode)
    {
        switch (filterMode)
        {
            case TINYGLTF_TEXTURE_FILTER_NEAREST: return TextureMagFilter::Nearest;
            case TINYGLTF_TEXTURE_FILTER_LINEAR: return TextureMagFilter::Linear;
        }

        return TextureMagFilter::Linear;
    }

    TextureMinFilter getMipFilter(int filterMode)
    {
        switch (filterMode)
        {
            case TINYGLTF_TEXTURE_FILTER_NEAREST: return TextureMinFilter::Nearest;
            case TINYGLTF_TEXTURE_FILTER_LINEAR: return TextureMinFilter::Linear;
            case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST: return TextureMinFilter::NearestMipmapNearest;
            case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST: return TextureMinFilter::LinearMipmapNearest;
            case TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR: return TextureMinFilter::NearestMipmapLinear;
            case TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR: return TextureMinFilter::LinearMipmapLinear;
        }

        return TextureMinFilter::Linear;
    }

    Texture createTexture(const VulkanRenderDevice* renderDevice, const std::shared_ptr<ImageData> imageData)
    {
        const LoadedImage& loadedImage = imageData->loadedImage;

        TextureSpecification textureSpecification {
            .format = loadedImage.format(),
            .width = static_cast<uint32_t>(loadedImage.width()),
            .height = static_cast<uint32_t>(loadedImage.height()),
            .layerCount = 1,
            .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
            .imageUsage =
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                VK_IMAGE_USAGE_SAMPLED_BIT,
            .imageAspect = VK_IMAGE_ASPECT_COLOR_BIT,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .magFilter = imageData->magFilter,
            .minFilter = imageData->minFilter,
            .wrapS = imageData->wrapModeS,
            .wrapT = imageData->wrapModeT,
            .wrapR = imageData->wrapModeT,
            .generateMipMaps = true
        };

        Texture texture {.name = imageData->loadedImage.path().filename().string()};

        VulkanTexture vkTexture(*renderDevice, textureSpecification, imageData->loadedImage.data());
        vkTexture.vulkanImage.transitionLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
        vkTexture.generateMipMaps();
        vkTexture.vulkanImage.transitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        vkTexture.setDebugName(texture.name);

        texture.vulkanTexture = std::move(vkTexture);

        return texture;
    }
}
