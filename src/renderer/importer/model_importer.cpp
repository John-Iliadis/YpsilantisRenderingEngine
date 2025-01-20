//
// Created by Gianni on 19/01/2025.
//

#include <stb/stb_image.h>
#include "model_importer.hpp"

static constexpr uint32_t sImportFlags
{
    aiProcess_CalcTangentSpace |
    aiProcess_JoinIdenticalVertices |
    aiProcess_Triangulate |
    aiProcess_RemoveComponent |
    aiProcess_GenNormals |
    aiProcess_ImproveCacheLocality |
    aiProcess_RemoveRedundantMaterials |
    aiProcess_SortByPType |
    aiProcess_GenUVCoords |
    aiProcess_OptimizeMeshes |
    aiProcess_OptimizeGraph
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

ModelImporter::ModelImporter()
    : mRenderDevice()
{
}

void ModelImporter::create(const VulkanRenderDevice& renderDevice)
{
    mRenderDevice = &renderDevice;
}

void ModelImporter::importModel(const std::string &path)
{
    mLoadedModelFutures.push_back(loadModel(path));
}

void ModelImporter::processMainThreadTasks()
{
    std::lock_guard<std::mutex> lock(mMainThreadTasksMutex);
    for (Task& task : mMainThreadTasks)
        task();
    mMainThreadTasks.clear();

    // get loaded model
    for (auto itr = mLoadedModelFutures.begin(); itr != mLoadedModelFutures.end();)
    {
        if (itr->wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            mOnModelImportFinished(*itr->get());
            itr = mLoadedModelFutures.erase(itr);
        }
        else
            ++itr;
    }
}

void ModelImporter::setImportFinishedCallback(std::function<void(const LoadedModel&)> callback)
{
    mOnModelImportFinished = callback;
}

void ModelImporter::enqueue(Task task)
{
    std::lock_guard<std::mutex> lock(mMainThreadTasksMutex);
    mMainThreadTasks.push_back(std::move(task));
}

LoadedModel::Mesh ModelImporter::createMesh(const LoadedMesh &loadedMesh)
{
    return {
        .mesh = std::make_shared<InstancedMesh>(*mRenderDevice,
                                                loadedMesh.vertices.size(),
                                                loadedMesh.vertices.data(),
                                                loadedMesh.indices.size(),
                                                loadedMesh.indices.data(),
                                                loadedMesh.name),
        .materialIndex = loadedMesh.materialIndex
    };
}

std::pair<std::string, NamedTexture> ModelImporter::createNamedTexturePair(const LoadedTexture<uint8_t> &loadedTexture)
{
    VulkanTexture texture = createTexture2D(*mRenderDevice,
                                            loadedTexture.width,
                                            loadedTexture.height,
                                            VK_FORMAT_R8G8B8A8_UNORM,
                                            VK_IMAGE_USAGE_SAMPLED_BIT |
                                                VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                                VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                                            VK_IMAGE_ASPECT_COLOR_BIT,
                                            TextureWrap::ClampToEdge,
                                            TextureFilter::Anisotropic,
                                            true);

    // set debug name
    setImageDebugName(*mRenderDevice, texture.image, loadedTexture.name.c_str());

    // upload data
    transitionImageLayout(*mRenderDevice,
                          texture.image,
                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    uploadTextureData(*mRenderDevice, texture, loadedTexture.pixels.get());

    // gen mip maps
    transitionImageLayout(*mRenderDevice,
                          texture.image,
                          VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
    generateMipMaps(*mRenderDevice, texture.image);

    // transition to shader read only
    transitionImageLayout(*mRenderDevice,
                          texture.image,
                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    return std::make_pair(loadedTexture.name, NamedTexture(loadedTexture.name, std::move(texture)));
}

std::future<std::shared_ptr<LoadedModel>> ModelImporter::loadModel(const fs::path& path)
{
    return std::async(std::launch::async, [this, path] () -> std::shared_ptr<LoadedModel> {
        std::shared_ptr<aiScene> assimpScene = loadAssimpScene(path);
        std::string directory = std::format("{}/", path.parent_path().string());

        std::shared_ptr<LoadedModel> model = std::make_shared<LoadedModel>();
        model->name = path.stem().string();
        model->root = createModelGraph(assimpScene->mRootNode);

        // load mesh data
        std::vector<std::future<LoadedMesh>> loadedMeshes;
        for (uint32_t i = 0; i < assimpScene->mNumMeshes; ++i)
        {
            const aiMesh& assimpMesh = *assimpScene->mMeshes[i];
            loadedMeshes.push_back(createLoadedMesh(assimpMesh));
        }

        // upload mesh data
        for (auto& loadedMesh : loadedMeshes)
        {
            enqueue([this, model, loadedMesh = loadedMesh.get()] () {
                model->meshes.push_back(this->createMesh(loadedMesh));
            });
        }

        // load materials
        std::unordered_set<std::string> texturePaths;
        for (uint32_t i = 0; i < assimpScene->mNumMaterials; ++i)
        {
            const aiMaterial& material = *assimpScene->mMaterials[i];
            LoadedModel::Material loadedMaterial = createMaterial(material, directory);

            for (auto& texturePath : loadedMaterial.textures)
            {
                if (!texturePath.empty())
                    texturePaths.insert(texturePath);
            }

            model->materials.push_back(std::move(loadedMaterial));
        }

        // load texture data
        std::vector<std::future<std::optional<LoadedTexture<uint8_t>>>> loadedTextures;
        for (const auto& texturePath : texturePaths)
            loadedTextures.push_back(createLoadedTexture(texturePath));

        // upload texture data
        for (auto& loadedTexture : loadedTextures)
        {
            enqueue([this, model, loadedTexture = loadedTexture.get()]() {
                if (loadedTexture.has_value())
                {
                    model->textures.emplace(this->createNamedTexturePair(loadedTexture.value()));
                }
            });
        }

        return model;
    });
}

std::shared_ptr<aiScene> ModelImporter::loadAssimpScene(const fs::path &path)
{
    Assimp::Importer importer;
    importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, sRemoveComponents);
    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, sRemovePrimitives);
    importer.SetPropertyBool(AI_CONFIG_PP_PTV_NORMALIZE, true);

    importer.ReadFile(path.string(), sImportFlags);

    std::shared_ptr<aiScene> assimpScene(importer.GetOrphanedScene());

    if (!assimpScene || assimpScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE)
    {
        debugLog(std::format("Failed to load model {}.", path.string()));
        return nullptr;
    }

    return assimpScene;
}

ModelNode ModelImporter::createModelGraph(const aiNode* assimpNode)
{
    ModelNode modelNode(assimpNode->mName.C_Str(), assimpToGlmMat4(assimpNode->mTransformation));

    for (uint32_t i = 0; i < assimpNode->mNumMeshes; ++i)
        modelNode.meshIndices.push_back(i);

    for (uint32_t i = 0; i < assimpNode->mNumChildren; ++i)
        modelNode.children.push_back(createModelGraph(assimpNode->mChildren[i]));

    return modelNode;
}

std::future<LoadedMesh> ModelImporter::createLoadedMesh(const aiMesh &mesh)
{
    return std::async(std::launch::async, [&mesh] () -> LoadedMesh {
        return {
            .name = mesh.mName.C_Str(),
            .vertices = ModelImporter::loadMeshVertices(mesh),
            .indices = ModelImporter::loadMeshIndices(mesh),
            .materialIndex = mesh.mMaterialIndex
        };
    });
}

// todo: enable debug comments
std::future<std::optional<LoadedTexture<uint8_t>>> ModelImporter::createLoadedTexture(const std::string &path)
{
    debugLog(std::format("ModelImporter::createLoadedTexture: Loading {}", path));
    return std::async(std::launch::async, [path = fs::path(path)] () -> std::optional<LoadedTexture<uint8_t>> {
        int width, height, channels;
        uint8_t* pixels = stbi_load(path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);

        if (pixels)
        {
            debugLog(std::format("ModelImporter::createLoadedTexture: Finished loading {}", path.string()));

            return std::make_optional<LoadedTexture<uint8_t>>(
                path.string(),
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height),
                pixels);
        }

        debugLog(std::format("ModelImporter::createLoadedTexture: Failed to load {}", path.string()));
        return {};
    });
}

// todo: check if the texture paths are correct
LoadedModel::Material ModelImporter::createMaterial(const aiMaterial &assimpMaterial, const std::string& directory)
{
    LoadedModel::Material material {.name = assimpMaterial.GetName().C_Str()};

    aiColor4D albedoColor;
    aiColor4D emissionColor;

    if (assimpMaterial.Get(AI_MATKEY_COLOR_DIFFUSE, emissionColor) == AI_SUCCESS)
        material.albedoColor = glm::vec3(albedoColor.r, albedoColor.g, albedoColor.b);
    if (assimpMaterial.Get(AI_MATKEY_COLOR_DIFFUSE, emissionColor) == AI_SUCCESS)
        material.emissionColor = glm::vec3(emissionColor.r, emissionColor.g, emissionColor.b);

    for (uint32_t i = TextureType::None + 1; i < TextureType::Count; ++i)
    {
        std::string textureName = getTextureName(assimpMaterial, static_cast<TextureType>(i));

        if (!textureName.empty())
            material.textures[i] = directory + textureName;
    }

    return material;
}

std::vector<InstancedMesh::Vertex> ModelImporter::loadMeshVertices(const aiMesh &mesh)
{
    std::vector<InstancedMesh::Vertex> vertices(mesh.mNumVertices);

    for (size_t i = 0; i < vertices.size(); ++i)
    {
        vertices.at(i).position = *reinterpret_cast<glm::vec3*>(&mesh.mVertices[i]);
        vertices.at(i).normal = *reinterpret_cast<glm::vec3*>(&mesh.mNormals[i]);

        if (mesh.HasTangentsAndBitangents())
        {
            vertices.at(i).tangent = *reinterpret_cast<glm::vec3*>(&mesh.mTangents[i]);
            vertices.at(i).bitangent = *reinterpret_cast<glm::vec3*>(&mesh.mBitangents[i]);
        }
        else
        {
            static glm::vec3 someVec = glm::vec3(1.f, 0.f, 0.f);
            vertices.at(i).tangent = glm::cross(vertices.at(i).normal, someVec);

            if (glm::length(vertices.at(i).tangent) < 0.0001f)
            {
                someVec = glm::vec3(0.f, 1.f, 0.f);
                vertices.at(i).tangent = glm::cross(vertices.at(i).normal, someVec);
            }

            vertices.at(i).bitangent = glm::cross(vertices.at(i).normal, vertices.at(i).bitangent);
        }

        if (mesh.HasTextureCoords(0))
        {
            vertices.at(i).texCoords = *reinterpret_cast<glm::vec2*>(&mesh.mTextureCoords[0][i]);
        }
    }

    return vertices;
}

std::vector<uint32_t> ModelImporter::loadMeshIndices(const aiMesh &mesh)
{
    std::vector<uint32_t> indices(mesh.mNumFaces * 3);

    for (uint32_t i = 0; i < mesh.mNumFaces; ++i)
    {
        const aiFace& face = mesh.mFaces[i];

        indices.at(i) = face.mIndices[0];
        indices.at(i + 1) = face.mIndices[1];
        indices.at(i + 2) = face.mIndices[2];
    }

    return indices;
}

glm::mat4 ModelImporter::assimpToGlmMat4(const aiMatrix4x4 &mat)
{
    return {
        mat.a1, mat.b1, mat.c1, mat.d1,
        mat.a2, mat.b2, mat.c2, mat.d2,
        mat.a3, mat.b3, mat.c3, mat.d3,
        mat.a4, mat.b4, mat.c4, mat.d4
    };
}

TextureType ModelImporter::getTextureType(aiTextureType type)
{
    switch (type)
    {
        case aiTextureType_DIFFUSE: return TextureType::Albedo;
        case aiTextureType_SPECULAR: return TextureType::Specular;
        case aiTextureType_EMISSIVE: return TextureType::Emission;
        case aiTextureType_HEIGHT: return TextureType::Normal;
        case aiTextureType_DISPLACEMENT: return TextureType::Displacement;
        case aiTextureType_METALNESS: return TextureType::Metallic;
        case aiTextureType_DIFFUSE_ROUGHNESS: return TextureType::Roughness;
        case aiTextureType_AMBIENT_OCCLUSION: return TextureType::Ao;
        default: assert(false);
    }
}

aiTextureType ModelImporter::getTextureType(TextureType type)
{
    switch (type)
    {
        case TextureType::Albedo: return aiTextureType_DIFFUSE;
        case TextureType::Specular: return aiTextureType_SPECULAR;
        case TextureType::Emission: return aiTextureType_EMISSIVE;
        case TextureType::Normal: return aiTextureType_HEIGHT;
        case TextureType::Displacement: return aiTextureType_DISPLACEMENT;
        case TextureType::Metallic: return aiTextureType_METALNESS;
        case TextureType::Roughness: return aiTextureType_DIFFUSE_ROUGHNESS;
        case TextureType::Ao: return aiTextureType_AMBIENT_OCCLUSION;
        default: assert(false);
    }
}

std::string ModelImporter::getTextureName(const aiMaterial &material, TextureType type)
{
    aiTextureType assimpTextureType = getTextureType(type);

    if (material.GetTextureCount(assimpTextureType))
    {
        aiString textureFilename;
        material.GetTexture(assimpTextureType, 0, &textureFilename);
        return textureFilename.C_Str();
    }

    return {};
}
