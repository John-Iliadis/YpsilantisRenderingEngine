//
// Created by Gianni on 23/01/2025.
//

#include "model_importer.hpp"

// todo: support specular glossiness
// todo: add opacity map

static constexpr uint32_t sImportFlags
{
    aiProcess_CalcTangentSpace |
    aiProcess_Triangulate |
    aiProcess_RemoveComponent |
    aiProcess_GenNormals |
    aiProcess_ImproveCacheLocality |
    aiProcess_RemoveRedundantMaterials |
    aiProcess_SortByPType |
    aiProcess_GenUVCoords |
    aiProcess_ValidateDataStructure |
    aiProcess_PreTransformVertices
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

ModelLoader::ModelLoader(const ModelImportData& importData)
    : path(importData.path)
    , root()
    , mSuccess()
{
    debugLog("Loading model: " + path.string());

    Assimp::Importer importer;
    importer.SetPropertyInteger(AI_CONFIG_PP_RVC_FLAGS, sRemoveComponents);
    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, sRemovePrimitives);
    importer.SetPropertyBool(AI_CONFIG_PP_PTV_NORMALIZE, importData.normalize);

    importer.ReadFile(path.string(), sImportFlags);

    if (importData.flipUVs) stbi_set_flip_vertically_on_load(true);

    const auto* scene = importer.GetScene();

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        debugLog("Failed to load model: " + path.string());
        debugLog("Importer error: " + std::string(importer.GetErrorString()));
        return;
    }

    try
    {
        createHierarchy(*scene);
        loadMeshes(*scene);

        getTextureNames(*scene);

        loadTextures(*scene);
        loadMaterials(*scene);

        mSuccess = true;
    }
    catch (const std::exception& e)
    {
        debugLog("Failed to load model: " + path.string());
        debugLog("Exception caught: " + std::string(e.what()));
    }

    debugLog("Successfully loaded: " + path.string());

    stbi_set_flip_vertically_on_load(false);
}

ModelLoader::ModelLoader(ModelLoader &&other) noexcept
    : ModelLoader()
{
    swap(other);
}

ModelLoader &ModelLoader::operator=(ModelLoader &&other) noexcept
{
    if (this != &other)
        swap(other);
    return *this;
}

void ModelLoader::swap(ModelLoader &other) noexcept
{
    std::swap(path, other.path);
    std::swap(root, other.root);
    std::swap(meshes, other.meshes);
    std::swap(images, other.images);
    std::swap(materials, other.materials);
    std::swap(materialNames, other.materialNames);
    std::swap(mTextureNames, other.mTextureNames);
    std::swap(mInsertedTexIndex, other.mInsertedTexIndex);
    std::swap(mSuccess, other.mSuccess);
}

bool ModelLoader::success() const
{
    return mSuccess;
}

void ModelLoader::createHierarchy(const aiScene &aiScene)
{
    root = createRootSceneNode(aiScene, *aiScene.mRootNode);
}

void ModelLoader::loadMeshes(const aiScene& aiScene)
{
    for (uint32_t i = 0; i < aiScene.mNumMeshes; ++i)
    {
        const aiMesh& aiMesh = *aiScene.mMeshes[i];
        const aiAABB& aabb = aiMesh.mAABB;
        const aiMaterial& aiMaterial = *aiScene.mMaterials[aiMesh.mMaterialIndex];

        debugLog(std::format("Loading mesh: {}", aiMesh.mName.data));

        auto center = (aabb.mMin + aabb.mMax) / 2.f;

        MeshData meshData {
            .name = aiMesh.mName.data,
            .vertices = loadMeshVertices(aiMesh),
            .indices = loadMeshIndices(aiMesh),
            .materialIndex = aiMesh.mMaterialIndex,
            .center = glm::make_vec3(&center.x)
        };

        meshes.push_back(std::move(meshData));
    }
}

void ModelLoader::loadTextures(const aiScene& aiScene)
{
    for (const std::string& texName : mTextureNames)
    {
        if (aiScene.GetEmbeddedTexture(texName.c_str()))
        {
            if (auto imageData = loadEmbeddedImageData(aiScene, texName))
            {
                mInsertedTexIndex.emplace(texName, images.size());
                images.push_back(std::move(*imageData));
            }
        }
        else
        {
            if (auto imageData = loadImageData(texName))
            {
                mInsertedTexIndex.emplace(texName, images.size());
                images.push_back(std::move(*imageData));
            }
        }
    }
}

void ModelLoader::loadMaterials(const aiScene& aiScene)
{
    for (uint32_t i = 0; i < aiScene.mNumMaterials; ++i)
    {
        const aiMaterial& aiMaterial = *aiScene.mMaterials[i];

        materials.push_back(loadMaterial(aiMaterial));

        aiString matName = aiMaterial.GetName();
        materialNames.push_back(matName.length? matName.data : "Unnamed Material");
    }
}

void ModelLoader::getTextureNames(const aiScene& aiScene)
{
    for (uint32_t i = 0; i < aiScene.mNumMaterials; ++i)
    {
        const aiMaterial& material = *aiScene.mMaterials[i];

        auto baseColTexName = getTextureName(material, aiTextureType_BASE_COLOR);
        auto normalTexName = getTextureName(material, aiTextureType_NORMALS);
        auto emissionTexName = getTextureName(material, aiTextureType_EMISSIVE);
        auto metallicTexName = getTextureName(material, aiTextureType_METALNESS);
        auto roughnessTexName = getTextureName(material, aiTextureType_DIFFUSE_ROUGHNESS);
        auto aoTexName = getTextureName(material, aiTextureType_LIGHTMAP);

        if (baseColTexName) mTextureNames.insert(*baseColTexName);
        if (normalTexName) mTextureNames.insert(*normalTexName);
        if (emissionTexName) mTextureNames.insert(*emissionTexName);
        if (metallicTexName) mTextureNames.insert(*metallicTexName);
        if (roughnessTexName) mTextureNames.insert(*roughnessTexName);
        if (aoTexName) mTextureNames.insert(*aoTexName);
    }
}

SceneNode ModelLoader::createRootSceneNode(const aiScene& aiScene, const aiNode& aiNode)
{
    glm::mat4 transformation = assimpToGlmMat4(aiNode.mTransformation);

    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;
    glm::vec3 skew;
    glm::vec4 perspective;

    glm::decompose(transformation, scale, rotation, translation, skew, perspective);

    glm::vec3 eulerAngles = glm::degrees(glm::eulerAngles(rotation));

    eulerAngles.y = eulerAngles.y == 0.f && std::signbit(eulerAngles.y)? 0.f : eulerAngles.y;

    SceneNode sceneNode {
        .name = aiNode.mName.length ? aiNode.mName.C_Str() : "Unnamed",
        .translation = translation,
        .rotation = eulerAngles,
        .scale = scale
    };

    if (uint32_t meshCount = aiNode.mNumMeshes)
    {
        sceneNode.meshIndices.insert(sceneNode.meshIndices.begin(),
                                     aiNode.mMeshes,
                                     aiNode.mMeshes + meshCount);
    }

    for (uint32_t i = 0; i < aiNode.mNumChildren; ++i)
    {
        const struct aiNode& aiChild = *aiNode.mChildren[i];
        sceneNode.children.push_back(createRootSceneNode(aiScene, aiChild));
    }

    return sceneNode;
}

glm::mat4 ModelLoader::assimpToGlmMat4(const aiMatrix4x4 &mat)
{
    return {
        mat.a1, mat.b1, mat.c1, mat.d1,
        mat.a2, mat.b2, mat.c2, mat.d2,
        mat.a3, mat.b3, mat.c3, mat.d3,
        mat.a4, mat.b4, mat.c4, mat.d4
    };
}

std::vector<Vertex> ModelLoader::loadMeshVertices(const aiMesh &aiMesh)
{
    std::vector<Vertex> vertices(aiMesh.mNumVertices);

    for (size_t i = 0; i < aiMesh.mNumVertices; ++i)
    {
        if (aiMesh.HasPositions())
        {
            vertices.at(i).position = *reinterpret_cast<glm::vec3*>(&aiMesh.mVertices[i]);
        }

        if (aiMesh.HasNormals())
        {
            vertices.at(i).normal = *reinterpret_cast<glm::vec3*>(&aiMesh.mNormals[i]);
        }

        if (aiMesh.HasTangentsAndBitangents())
        {
            vertices.at(i).tangent = *reinterpret_cast<glm::vec3*>(&aiMesh.mTangents[i]);
            vertices.at(i).bitangent = *reinterpret_cast<glm::vec3*>(&aiMesh.mBitangents[i]);
        }

        if (aiMesh.HasTextureCoords(0))
        {
            vertices.at(i).texCoords = *reinterpret_cast<glm::vec2*>(&aiMesh.mTextureCoords[0][i]);
        }
    }

    return vertices;
}

std::vector<uint32_t> ModelLoader::loadMeshIndices(const aiMesh &aiMesh)
{
    std::vector<uint32_t> indices;

    if (aiMesh.HasFaces())
    {
        indices.reserve(aiMesh.mNumFaces * 3);

        for (uint32_t i = 0; i < aiMesh.mNumFaces; ++i)
        {
            const aiFace& face = aiMesh.mFaces[i];

            indices.push_back(face.mIndices[0]);
            indices.push_back(face.mIndices[1]);
            indices.push_back(face.mIndices[2]);
        }
    }

    return indices;
}

std::optional<ImageData> ModelLoader::loadImageData(const std::string &texName)
{
    std::filesystem::path texPath = path.parent_path() / texName;

    debugLog("Loading image: " + texPath.string());

    LoadedImage loadedImage(texPath);

    if (!loadedImage.success())
    {
        debugLog("Failed to load image: " + texPath.string());
        return std::nullopt;
    }

    uint8_t* data = reinterpret_cast<uint8_t*>(loadedImage.getOrphanedData());

    return ImageData {
        .width = loadedImage.width(),
        .height = loadedImage.height(),
        .name = texPath.filename().string(),
        .imageData {data, [] (uint8_t* d) { stbi_image_free(d); }},
        .magFilter = TextureMagFilter::Linear,
        .minFilter = TextureMinFilter::Linear,
        .wrapModeS = TextureWrap::Repeat,
        .wrapModeT = TextureWrap::Repeat
    };
}

std::optional<ImageData> ModelLoader::loadEmbeddedImageData(const aiScene& aiScene, const std::string& texName)
{
    aiTexture& aiTex = const_cast<aiTexture &>(*aiScene.GetEmbeddedTexture(texName.data()));

    debugLog("Loading embedded image: " + texName);

    ImageData imageData {
        .name = aiTex.mFilename.length? aiTex.mFilename.data : "Embedded Texture",
        .magFilter = TextureMagFilter::Linear,
        .minFilter = TextureMinFilter::Linear,
        .wrapModeS = TextureWrap::Repeat,
        .wrapModeT = TextureWrap::Repeat,
    };

    uint8_t* data;
    std::function<void(uint8_t*)> deleter;
    if (aiTex.mHeight == 0)
    {
        data = stbi_load_from_memory(
            reinterpret_cast<uint8_t*>(aiTex.pcData),
            aiTex.mWidth,
            &imageData.width,
            &imageData.height,
            nullptr, 4);

        deleter = [] (uint8_t* d) { stbi_image_free(d); };
    }
    else
    {
        imageData.width = aiTex.mWidth;
        imageData.height = aiTex.mHeight;

        data = reinterpret_cast<uint8_t*>(aiTex.pcData);

        aiTex.pcData = nullptr;

        deleter = [] (uint8_t* d) { delete[] d; };
    }

    if (!data)
    {
        debugLog("Failed to load embedded image: " + texName);
        return std::nullopt;
    }

    imageData.imageData = {data, deleter};

    return imageData;
}

std::optional<std::string> ModelLoader::getTextureName(const aiMaterial &aiMaterial, aiTextureType type)
{
    if (aiMaterial.GetTextureCount(type))
    {
        aiString filename;

        aiMaterial.GetTexture(type, 0, &filename);

        return filename.data;
    }

    return std::nullopt;
}

Material ModelLoader::loadMaterial(const aiMaterial &aiMaterial)
{
    auto getTexIndex = [this, &aiMaterial] (aiTextureType type) {
        if (auto texName = getTextureName(aiMaterial, type))
            if (mInsertedTexIndex.contains(*texName))
                return mInsertedTexIndex.at(*texName);
        return -1;
    };

    Material material {
        .baseColorTexIndex = getTexIndex(aiTextureType_BASE_COLOR),
        .metallicTexIndex = getTexIndex(aiTextureType_METALNESS),
        .roughnessTexIndex = getTexIndex(aiTextureType_DIFFUSE_ROUGHNESS),
        .normalTexIndex = getTexIndex(aiTextureType_NORMALS),
        .aoTexIndex = getTexIndex(aiTextureType_LIGHTMAP),
        .emissionTexIndex = getTexIndex(aiTextureType_EMISSIVE),
        .metallicFactor = getMetallicFactor(aiMaterial),
        .roughnessFactor = getRoughnessFactor(aiMaterial),
        .occlusionStrength = 1.f,
        .emissionFactor = 1.f,
        .alphaCutoff = getAlphaCutoff(aiMaterial),
        .alphaMode = getAlphaMode(aiMaterial),
        .baseColorFactor = getBaseColorFactor(aiMaterial),
        .emissionColor = getEmissionFactor(aiMaterial),
        .tiling = glm::vec4(1.f),
        .offset = glm::vec3(0.f)
    };

    return material;
}

glm::vec4 ModelLoader::getBaseColorFactor(const aiMaterial &aiMaterial)
{
    static constexpr glm::vec4 defaultBaseColorFactor(1.f, 1.f, 1.f, 1.f);

    aiColor4D aiBaseColor;

    if (aiMaterial.Get(AI_MATKEY_BASE_COLOR, aiBaseColor) == aiReturn_SUCCESS)
        return *reinterpret_cast<glm::vec4*>(&aiBaseColor);

    return defaultBaseColorFactor;
}

glm::vec4 ModelLoader::getEmissionFactor(const aiMaterial &aiMaterial)
{
    static constexpr glm::vec4 defaultEmissionFactor(0.f, 0.f, 0.f, 0.f);

    aiColor3D aiEmissionColor;

    if (aiMaterial.Get(AI_MATKEY_COLOR_EMISSIVE, aiEmissionColor) == aiReturn_SUCCESS)
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

float ModelLoader::getMetallicFactor(const aiMaterial &aiMaterial)
{
    static constexpr float defaultMetallicFactor = 1.f;

    ai_real metallicFactor;

    if (aiMaterial.Get(AI_MATKEY_METALLIC_FACTOR, metallicFactor) == aiReturn_SUCCESS)
        return metallicFactor;

    return defaultMetallicFactor;
}

float ModelLoader::getRoughnessFactor(const aiMaterial &aiMaterial)
{
    static constexpr float defaultRoughnessFactor = 1.f;

    ai_real roughnessFactor;

    if (aiMaterial.Get(AI_MATKEY_ROUGHNESS_FACTOR, roughnessFactor) == aiReturn_SUCCESS)
        return roughnessFactor;

    return defaultRoughnessFactor;
}

float ModelLoader::getAlphaCutoff(const aiMaterial &aiMaterial)
{
    static constexpr float defaultAlphaCutoff = 0.5f;

    float alphaCutoff;
    if (aiMaterial.Get(AI_MATKEY_GLTF_ALPHACUTOFF, alphaCutoff) == AI_SUCCESS)
        return alphaCutoff;

    return defaultAlphaCutoff;
}

AlphaMode ModelLoader::getAlphaMode(const aiMaterial &aiMaterial)
{
    aiString alphaMode;
    if (aiMaterial.Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == AI_SUCCESS)
    {
        if (!strcmp(alphaMode.C_Str(), "MASK"))
            return AlphaMode::Mask;
        else if (!strcmp(alphaMode.C_Str(), "BLEND"))
            return AlphaMode::Blend;
    }

    return AlphaMode::Opaque;
}
