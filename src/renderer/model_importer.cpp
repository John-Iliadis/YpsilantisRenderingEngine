//
// Created by Gianni on 23/01/2025.
//

#include "model_importer.hpp"

// todo: handle embedded image data
// todo: support specular glossiness
// todo: load sampler data
// todo: handle multiple texture coordinates
// todo: handle color vertices. Including alpha channel

static constexpr uint32_t sImportFlags
{
    aiProcess_CalcTangentSpace |
    aiProcess_Triangulate |
    aiProcess_RemoveComponent |
    aiProcess_GenNormals |
    aiProcess_ImproveCacheLocality |
    aiProcess_RemoveRedundantMaterials |
    aiProcess_SortByPType |
    aiProcess_GenUVCoords
};

static constexpr int sRemoveComponents
{
    aiComponent_BONEWEIGHTS |
    aiComponent_ANIMATIONS |
    aiComponent_LIGHTS |
    aiComponent_CAMERAS |
    aiComponent_COLORS
};

static constexpr int sRemovePrimitives
{
    aiPrimitiveType_POINT |
    aiPrimitiveType_LINE
};

namespace ModelImporter
{
    std::future<std::shared_ptr<Model>> loadModel(const std::filesystem::path& path, const VulkanRenderDevice* renderDevice, EnqueueCallback callback)
    {
        return std::async(std::launch::async, [path, renderDevice, callback] () {
            std::shared_ptr<aiScene> assimpScene = loadScene(path);
            std::shared_ptr<Model> model = std::make_shared<Model>(renderDevice);

            model->name = path.filename().string();
            model->path = path;
            model->root = createRootSceneNode(*assimpScene->mRootNode);

            // load mesh data
            std::vector<std::future<MeshData>> meshDataFutures;
            for (uint32_t i = 0; i < assimpScene->mNumMeshes; ++i)
            {
                const aiMesh& assimpMesh = *assimpScene->mMeshes[i];
                meshDataFutures.push_back(loadMeshData(assimpMesh));
            }

            // upload mesh data to vulkan
            for (auto& meshDataFuture : meshDataFutures)
            {
                callback([renderDevice, model, meshData = meshDataFuture.get()] () {
                    model->meshes.push_back(createMesh(renderDevice, meshData));
                });
            }

            // load texture data
            std::unordered_set<std::string> texNames = getTextureNames(*assimpScene);
            std::filesystem::path dir = path.parent_path();
            std::vector<std::future<std::shared_ptr<ImageData>>> loadedImageFutures;

            for (const auto& texFilename : texNames)
            {
                loadedImageFutures.push_back(loadImageData(dir / texFilename));
            }

            // upload texture data to vulkan
            std::unordered_map<std::filesystem::path, int32_t> texIndices;
            for (uint32_t i = 0, insertIndex = 0; i < loadedImageFutures.size(); ++i)
            {
                auto loadedImageData = loadedImageFutures.at(i).get();

                std::filesystem::path texPath = loadedImageData->loadedImage.path();

                if (loadedImageData->loadedImage.success())
                {
                    callback([renderDevice, model, loadedImageData] () {
                        model->textures.push_back(createTexture(renderDevice, loadedImageData));
                    });

                    texIndices.emplace(texPath, insertIndex++);
                    continue;
                }

                texIndices.emplace(texPath, -1);
            }

            // load materials
            MaterialData materialData = loadMaterials(*assimpScene, texIndices, dir);

            model->materials = std::move(materialData.materials);
            model->materialNames = std::move(materialData.materialNames);

            return model;
        });
    }

    std::shared_ptr<aiScene> loadScene(const std::filesystem::path& path)
    {
        Assimp::Importer importer;

        importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, sRemoveComponents);
        importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, sRemovePrimitives);
        importer.SetPropertyBool(AI_CONFIG_PP_PTV_NORMALIZE, true);

        importer.ReadFile(path.string(), sImportFlags);

        std::shared_ptr<aiScene> scene(importer.GetOrphanedScene());

        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
        {
            debugLog("Failed to load Assimp scene.");
            return nullptr;
        }

        return scene;
    }

    SceneNode createRootSceneNode(const aiNode& assimpNode)
    {
        SceneNode sceneNode {
            .name = assimpNode.mName.length? assimpNode.mName.C_Str() : "Unnamed",
            .transformation = assimpToGlmMat4(assimpNode.mTransformation)
        };

        if (uint32_t meshCount = assimpNode.mNumMeshes)
        {
            sceneNode.meshIndices.insert(sceneNode.meshIndices.begin(),
                                         assimpNode.mMeshes,
                                         assimpNode.mMeshes + meshCount);
        }

        for (uint32_t i = 0; i < assimpNode.mNumChildren; ++i)
        {
            const aiNode& child = *assimpNode.mChildren[i];
            sceneNode.children.push_back(createRootSceneNode(child));
        }

        return sceneNode;
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

    std::future<MeshData> loadMeshData(const aiMesh& assimpMesh)
    {
        debugLog(std::format("Loading mesh: {}", assimpMesh.mName.C_Str()));

        return std::async(std::launch::async, [&assimpMesh] () {

            MeshData meshData {
                .name = assimpMesh.mName.C_Str(),
                .vertices = loadMeshVertices(assimpMesh),
                .indices = loadMeshIndices(assimpMesh),
                .materialIndex = static_cast<int32_t>(assimpMesh.mMaterialIndex)
            };

            return meshData;
        });
    }

    std::vector<Vertex> loadMeshVertices(const aiMesh& assimpMesh)
    {
        std::vector<Vertex> vertices(assimpMesh.mNumVertices);
        
        for (size_t i = 0; i < assimpMesh.mNumVertices; ++i)
        {
            if (assimpMesh.HasPositions())
            {
                vertices.at(i).position = *reinterpret_cast<glm::vec3*>(&assimpMesh.mVertices[i]);
            }

            if (assimpMesh.HasNormals())
            {
                vertices.at(i).normal = *reinterpret_cast<glm::vec3*>(&assimpMesh.mNormals[i]);
            }

            if (assimpMesh.HasTangentsAndBitangents())
            {
                vertices.at(i).tangent = *reinterpret_cast<glm::vec3*>(&assimpMesh.mTangents[i]);
                vertices.at(i).bitangent = *reinterpret_cast<glm::vec3*>(&assimpMesh.mBitangents[i]);
            }

            if (assimpMesh.HasTextureCoords(0))
            {
                vertices.at(i).texCoords = *reinterpret_cast<glm::vec2*>(&assimpMesh.mTextureCoords[0][i]);
            }
        }

        return vertices;
    }

    std::vector<uint32_t> loadMeshIndices(const aiMesh& assimpMesh)
    {
        std::vector<uint32_t> indices;

        if (assimpMesh.HasFaces())
        {
            indices.reserve(assimpMesh.mNumFaces * 3);

            for (uint32_t i = 0; i < assimpMesh.mNumFaces; ++i)
            {
                const aiFace& face = assimpMesh.mFaces[i];

                indices.push_back(face.mIndices[0]);
                indices.push_back(face.mIndices[1]);
                indices.push_back(face.mIndices[2]);
            }
        }

        return indices;
    }

    std::future<std::shared_ptr<ImageData>> loadImageData(const std::filesystem::path& path)
    {
        debugLog("Loading texture: " + path.string());

        return std::async(std::launch::async, [path] {
            std::shared_ptr<ImageData> imageData = std::make_shared<ImageData>();

            imageData->loadedImage = LoadedImage(path);
            imageData->magFilter = TextureMagFilter::Linear;
            imageData->minFilter = TextureMinFilter::Linear;
            imageData->wrapModeS = TextureWrap::Repeat;
            imageData->wrapModeT = TextureWrap::Repeat;

            return imageData;
        });
    }

    Texture createTexture(const VulkanRenderDevice* renderDevice, std::shared_ptr<ImageData> imageData)
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

        vkTexture.transitionLayout(VK_IMAGE_LAYOUT_UNDEFINED,
                                   VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                   VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                   VK_PIPELINE_STAGE_TRANSFER_BIT,
                                   0, VK_ACCESS_TRANSFER_WRITE_BIT);

        vkTexture.generateMipMaps();

        vkTexture.transitionLayout(VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                   VK_PIPELINE_STAGE_TRANSFER_BIT,
                                   VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                   VK_ACCESS_TRANSFER_WRITE_BIT,
                                   VK_ACCESS_SHADER_READ_BIT);

        vkTexture.setDebugName(texture.name);

        texture.vulkanTexture = std::move(vkTexture);

        return texture;
    }

    MaterialData loadMaterials(const aiScene& assimpScene, const std::unordered_map<std::filesystem::path, int32_t>& texIndices, const std::filesystem::path& dir)
    {
        MaterialData materialData;

        for (uint32_t i = 0; i < assimpScene.mNumMaterials; ++i)
        {
            const aiMaterial& assimpMaterial = *assimpScene.mMaterials[i];

            materialData.materials.push_back(loadMaterial(assimpMaterial, texIndices, dir));
            materialData.materialNames.emplace_back(assimpMaterial.GetName().C_Str());
        }

        return materialData;
    }

    Material loadMaterial(const aiMaterial& assimpMaterial, const std::unordered_map<std::filesystem::path, int32_t>& texIndices, const std::filesystem::path& dir)
    {
        auto getTexIndex = [&assimpMaterial, &texIndices, &dir] (aiTextureType type) {
            if (auto texName = getTextureName(assimpMaterial, type))
            {
                std::filesystem::path path = dir / *texName;

                if (texIndices.contains(path))
                    return texIndices.at(path);
            }

            return -1;
        };

        Material material {
            .baseColorTexIndex = getTexIndex(aiTextureType_BASE_COLOR),
            .metallicTexIndex = getTexIndex(aiTextureType_METALNESS),
            .roughnessTexIndex = getTexIndex(aiTextureType_DIFFUSE_ROUGHNESS),
            .normalTexIndex = getTexIndex(aiTextureType_NORMALS),
            .aoTexIndex = getTexIndex(aiTextureType_AMBIENT_OCCLUSION),
            .emissionTexIndex = getTexIndex(aiTextureType_EMISSIVE),
            .metallicFactor = getMetallicFactor(assimpMaterial),
            .roughnessFactor = getRoughnessFactor(assimpMaterial),
            .baseColorFactor = getBaseColorFactor(assimpMaterial),
            .emissionFactor = getEmissionFactor(assimpMaterial),
            .tiling = glm::vec4(1.f),
            .offset = glm::vec3(0.f)
        };

        return material;
    }

    glm::vec4 getBaseColorFactor(const aiMaterial& assimpMaterial)
    {
        static constexpr glm::vec4 defaultBaseColorFactor(1.f, 1.f, 1.f, 1.f);

        aiColor4D aiBaseColor;

        if (assimpMaterial.Get(AI_MATKEY_BASE_COLOR, aiBaseColor) == aiReturn_SUCCESS)
            return *reinterpret_cast<glm::vec4*>(&aiBaseColor);

        return defaultBaseColorFactor;
    }

    glm::vec4 getEmissionFactor(const aiMaterial& assimpMaterial)
    {
        static constexpr glm::vec4 defaultEmissionFactor(0.f, 0.f, 0.f, 0.f);

        aiColor3D aiEmissionColor;

        if (assimpMaterial.Get(AI_MATKEY_COLOR_EMISSIVE, aiEmissionColor) == aiReturn_SUCCESS)
        {
            return {
                aiEmissionColor.r,
                aiEmissionColor.g,
                aiEmissionColor.b,
                0.f
            };
        }

        return defaultEmissionFactor;
    }

    float getMetallicFactor(const aiMaterial& assimpMaterial)
    {
        static constexpr float defaultMetallicFactor = 1.f;

        ai_real metallicFactor;

        if (assimpMaterial.Get(AI_MATKEY_METALLIC_FACTOR, metallicFactor) == aiReturn_SUCCESS)
            return metallicFactor;

        return defaultMetallicFactor;
    }

    float getRoughnessFactor(const aiMaterial& assimpMaterial)
    {
        static constexpr float defaultRoughnessFactor = 1.f;

        ai_real roughnessFactor;

        if (assimpMaterial.Get(AI_MATKEY_ROUGHNESS_FACTOR, roughnessFactor) == aiReturn_SUCCESS)
            return roughnessFactor;

        return defaultRoughnessFactor;
    }

    // todo: texture name might be key to embedded texture data
    std::unordered_set<std::string> getTextureNames(const aiScene& assimpScene)
    {
        std::unordered_set<std::string> texNames;

        auto addTexture = [&texNames] (const std::optional<std::string>& texName) {
            if (texName.has_value() && !texNames.contains(*texName))
                texNames.insert(*texName);
        };

        for (uint32_t i = 0; i < assimpScene.mNumMaterials; ++i)
        {
            const aiMaterial& material = *assimpScene.mMaterials[i];

            addTexture(getTextureName(material, aiTextureType_BASE_COLOR));
            addTexture(getTextureName(material, aiTextureType_NORMALS));
            addTexture(getTextureName(material, aiTextureType_EMISSION_COLOR));
            addTexture(getTextureName(material, aiTextureType_METALNESS));
            addTexture(getTextureName(material, aiTextureType_DIFFUSE_ROUGHNESS));
            addTexture(getTextureName(material, aiTextureType_AMBIENT_OCCLUSION));
        }

        return texNames;
    }

    std::optional<std::string> getTextureName(const aiMaterial& material, aiTextureType type)
    {
        if (material.GetTextureCount(type))
        {
            aiString filename;
            material.GetTexture(type, 0, &filename);
            return filename.C_Str();
        }

        return std::nullopt;
    }

    glm::mat4 assimpToGlmMat4(const aiMatrix4x4 &mat)
    {
        return {
            mat.a1, mat.b1, mat.c1, mat.d1,
            mat.a2, mat.b2, mat.c2, mat.d2,
            mat.a3, mat.b3, mat.c3, mat.d3,
            mat.a4, mat.b4, mat.c4, mat.d4
        };
    }
}
