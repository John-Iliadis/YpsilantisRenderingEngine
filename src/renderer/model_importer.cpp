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

ModelLoader::ModelLoader(const VulkanRenderDevice& renderDevice, const ModelImportData& importData)
    : path(importData.path)
    , root()
    , mRenderDevice(renderDevice)
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

    stbi_set_flip_vertically_on_load(false);
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
        const aiMaterial& aiMaterial = *aiScene.mMaterials[aiMesh.mMaterialIndex];

        debugLog(std::format("Loading mesh: {}", aiMesh.mName.data));

        MeshData meshData {
            .name = aiMesh.mName.data,
            .vertexStagingBuffer = loadVertexData(aiMesh),
            .indexStagingBuffer = loadIndexData(aiMesh),
            .materialIndex = aiMesh.mMaterialIndex,
            .opaque = isOpaque(aiMaterial)
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

VulkanBuffer ModelLoader::loadVertexData(const aiMesh &aiMesh)
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

    return {mRenderDevice,
            vertices.size() * sizeof(Vertex),
            BufferType::Staging,
            MemoryType::HostCached,
            vertices.data()};
}

VulkanBuffer ModelLoader::loadIndexData(const aiMesh &aiMesh)
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

    return {mRenderDevice,
            indices.size() * sizeof(uint32_t),
            BufferType::Staging,
            MemoryType::HostCached,
            indices.data()};
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

    int32_t texWidth = loadedImage.width();
    int32_t texHeight = loadedImage.height();

    VkDeviceSize deviceSize = texWidth * texHeight * formatSize(loadedImage.format());

    return ImageData {
        .width = loadedImage.width(),
        .height = loadedImage.height(),
        .name = texPath.filename().string(),
        .stagingBuffer {mRenderDevice, deviceSize, BufferType::Staging, MemoryType::HostCached, loadedImage.data()},
        .magFilter = TextureMagFilter::Linear,
        .minFilter = TextureMinFilter::Linear,
        .wrapModeS = TextureWrap::Repeat,
        .wrapModeT = TextureWrap::Repeat
    };
}

std::optional<ImageData> ModelLoader::loadEmbeddedImageData(const aiScene& aiScene, const std::string& texName)
{
    const aiTexture& aiTexture = *aiScene.GetEmbeddedTexture(texName.data());

    debugLog("Loading embedded image: " + texName);

    ImageData imageData {
        .name = aiTexture.mFilename.length? aiTexture.mFilename.data : "Embedded Texture",
        .magFilter = TextureMagFilter::Linear,
        .minFilter = TextureMinFilter::Linear,
        .wrapModeS = TextureWrap::Repeat,
        .wrapModeT = TextureWrap::Repeat,
    };

    const void* data = nullptr;
    if (aiTexture.mHeight == 0)
    {
        data = stbi_load_from_memory(
            reinterpret_cast<uint8_t*>(aiTexture.pcData),
            aiTexture.mWidth,
            &imageData.width,
            &imageData.height,
            nullptr, 4);
    }
    else
    {
        imageData.width = aiTexture.mWidth;
        imageData.height = aiTexture.mHeight;

        data = aiTexture.pcData;
    }

    if (!data)
    {
        debugLog("Failed to load embedded image: " + texName);
        return std::nullopt;
    }

    VkDeviceSize deviceSize = imageMemoryDeviceSize(imageData.width, imageData.height, VK_FORMAT_R8G8B8A8_UNORM);

    imageData.stagingBuffer = VulkanBuffer(mRenderDevice, deviceSize, BufferType::Staging, MemoryType::HostCached, data);

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
        .emissionFactor = 1.f,
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

bool ModelLoader::isOpaque(const aiMaterial &aiMaterial)
{
    float opacity;
    if (aiMaterial.Get(AI_MATKEY_OPACITY, opacity) != AI_SUCCESS)
        if (opacity < 1.f)
            return false;

    aiString alphaMode;
    if (aiMaterial.Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == AI_SUCCESS)
        if (!strcmp(alphaMode.C_Str(), "BLEND") || !strcmp(alphaMode.C_Str(), "MASK"))
            return false;

    return true;
}

ModelImporter::ModelImporter(const VulkanRenderDevice &renderDevice)
    : SubscriberSNS({Topic::Type::Renderer})
    , mRenderDevice(renderDevice)
{
}

void ModelImporter::update()
{
    for (auto itr = mModelDataFutures.begin(); itr != mModelDataFutures.end();)
    {
        if (itr->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            std::unique_ptr<ModelLoader> modelData = itr->second.get();

            if (modelData->success())
                addModel(modelData);

            itr = mModelDataFutures.erase(itr);
        }
        else
            ++itr;
    }

    for (auto itr = mFences.begin(); itr != mFences.end();)
    {
        if (vkGetFenceStatus(mRenderDevice.device, itr->second) == VK_SUCCESS)
        {
            auto model = mModels.at(itr->first);
            SNS::publishMessage(Topic::Type::Resource, Message::modelLoaded(model));

            mModelData.erase(itr->first);
            mModels.erase(itr->first);

            vkDestroyFence(mRenderDevice.device, itr->second, nullptr);

            itr = mFences.erase(itr);
        }
        else
            ++itr;
    }
}

void ModelImporter::notify(const Message &message)
{
    if (const auto m = message.getIf<Message::LoadModel>())
    {
        ModelImportData importData {
            .path = m->path.string(),
            .normalize = m->normalize,
            .flipUVs = m->flipUVs
        };

        auto future = std::async(std::launch::async, [this, importData] () {
            return std::make_unique<ModelLoader>(mRenderDevice, importData);
        });

        mModelDataFutures.emplace(m->path, std::move(future));
    }
}

void ModelImporter::addModel(std::unique_ptr<ModelLoader>& modelData)
{
    auto model = std::make_shared<Model>(mRenderDevice);

    model->id = UUIDRegistry::generateModelID();
    model->name = modelData->path.filename().string();
    model->path = modelData->path;
    model->root = std::move(modelData->root);
    model->materials = std::move(modelData->materials);
    model->materialNames = std::move(modelData->materialNames);

    VkCommandBuffer commandBuffer;
    VkCommandBufferAllocateInfo commandBufferAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = mRenderDevice.commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    VkResult result = vkAllocateCommandBuffers(mRenderDevice.device, &commandBufferAllocateInfo, &commandBuffer);
    vulkanCheck(result, "Failed to allocate command buffer.");

    VkCommandBufferBeginInfo commandBufferBeginInfo {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
    };

    vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
    addMeshes(commandBuffer, *model, *modelData);
    addTextures(commandBuffer, *model, *modelData);
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .commandBufferCount = 1,
        .pCommandBuffers = &commandBuffer
    };

    VkFence fence = createFence(mRenderDevice);

    result = vkQueueSubmit(mRenderDevice.graphicsQueue, 1, &submitInfo, fence);
    vulkanCheck(result, "Failed queue submission.");

    mModelData.emplace(model->path, std::move(modelData));
    mModels.emplace(model->path, model);
    mFences.emplace(model->path, fence);
}

void ModelImporter::addMeshes(VkCommandBuffer commandBuffer, Model &model, const ModelLoader &modelData)
{
    for (const auto& meshData : modelData.meshes)
    {
        VulkanBuffer vertexBuffer(mRenderDevice, meshData.vertexStagingBuffer.getSize(), BufferType::Vertex, MemoryType::Device);
        VulkanBuffer indexBuffer(mRenderDevice, meshData.indexStagingBuffer.getSize(), BufferType::Index, MemoryType::Device);

        vertexBuffer.copyBuffer(commandBuffer, meshData.vertexStagingBuffer, 0, 0, vertexBuffer.getSize());
        indexBuffer.copyBuffer(commandBuffer, meshData.indexStagingBuffer, 0, 0, indexBuffer.getSize());

        Mesh mesh {
            .meshID = UUIDRegistry::generateMeshID(),
            .name = meshData.name,
            .mesh {mRenderDevice, std::move(vertexBuffer), std::move(indexBuffer)},
            .materialIndex = meshData.materialIndex,
            .opaque = meshData.opaque
        };

        model.meshes.push_back(std::move(mesh));
    }
}

void ModelImporter::addTextures(VkCommandBuffer commandBuffer, Model &model, const ModelLoader &modelData)
{
    for (const auto& imageData : modelData.images)
    {
        TextureSpecification textureSpecification {
            .format = VK_FORMAT_R8G8B8A8_UNORM,
            .width = static_cast<uint32_t>(imageData.width),
            .height = static_cast<uint32_t>(imageData.height),
            .layerCount = 1,
            .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
            .imageUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                VK_IMAGE_USAGE_SAMPLED_BIT,
            .imageAspect = VK_IMAGE_ASPECT_COLOR_BIT,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .magFilter = imageData.magFilter,
            .minFilter = imageData.minFilter,
            .wrapS = imageData.wrapModeS,
            .wrapT = imageData.wrapModeT,
            .wrapR = imageData.wrapModeT,
            .generateMipMaps = true
        };

        VulkanTexture texture(mRenderDevice, textureSpecification);

        texture.transitionLayout(commandBuffer,
                                 VK_IMAGE_LAYOUT_UNDEFINED,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 0,
                                 VK_ACCESS_TRANSFER_WRITE_BIT);

        texture.copyBuffer(commandBuffer, imageData.stagingBuffer);

        texture.transitionLayout(commandBuffer,
                                 VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_ACCESS_TRANSFER_WRITE_BIT,
                                 VK_ACCESS_TRANSFER_READ_BIT);

        texture.generateMipMaps(commandBuffer);

        texture.transitionLayout(commandBuffer,
                                 VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                 VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                 VK_PIPELINE_STAGE_TRANSFER_BIT,
                                 VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                                 VK_ACCESS_TRANSFER_READ_BIT,
                                 VK_ACCESS_SHADER_READ_BIT);

        texture.setDebugName(imageData.name);

        Texture tex {
            .name = imageData.name,
            .vulkanTexture = std::move(texture)
        };

        model.textures.push_back(std::move(tex));
    }
}
