//
// Created by Gianni on 23/01/2025.
//

#include "resource_manager.hpp"

#define MAX_MATERIALS 128

template <typename T>
static bool resourceLoaded(const std::unordered_map<T, std::filesystem::path>& resourceMap, const std::filesystem::path& path)
{
    return std::any_of(resourceMap.begin(), resourceMap.end(), [&path] (const auto& pair) {
        return pair.second == path;
    });
}

template <typename T>
static std::optional<uuid64_t> getTID(const std::unordered_map<uuid64_t, T>& resourceMap, const T& t)
{
    for (const auto& [id, item] : resourceMap)
        if (item == t)
            return id;
    return {};
}

ResourceManager::ResourceManager(const std::shared_ptr<VulkanRenderDevice> renderDevice)
    : SubscriberSNS({Topic::Type::Resources, Topic::Type::SceneGraph})
    , mDSLayoutBuilder(*renderDevice)
    , mDSBuilder(*renderDevice)
    , mRenderDevice(renderDevice)
    , mMaterialsSSBO(*renderDevice, MAX_MATERIALS * sizeof(Material), BufferType::Storage, MemoryType::GPU)
{
    createDescriptorResources();
    loadDefaultTextures();
    loadDefaultMaterial();
}

ResourceManager::~ResourceManager()
{
    vkDestroyDescriptorSetLayout(mRenderDevice->device, mMaterialTexturesDescriptorSetLayout, nullptr);
    vkDestroyDescriptorSetLayout(mRenderDevice->device, mTexture2dDescriptorSetLayout, nullptr);
}

bool ResourceManager::importModel(const std::filesystem::path &path)
{
    if (resourceLoaded(mModelPaths, path))
    {
        debugLog(std::format("Model \"{}\" is already loaded.", path.string()));
        return false;
    }

    EnqueueCallback callback = [this] (Task&& t) {mTaskQueue.push(std::move(t));};
    mLoadedModelFutures.push_back(ResourceImporter::loadModel(path, mRenderDevice, callback));

    return true;
}

void ResourceManager::notify(const Message &message)
{
    if (const auto m = message.getIf<Message::MeshInstanceUpdate>())
    {
        mMeshes.at(m->meshID)->updateInstance(m->instanceID, m->transformation, m->objectID, m->matIndex);
    }

    if (const auto m = message.getIf<Message::RemoveMeshInstance>())
    {
        mMeshes.at(m->meshID)->removeInstance(m->instanceID);
    }

    if (const auto m = message.getIf<Message::MaterialDeleted>())
    {
        for (auto& [materialID, materialIndex] : mMaterials)
        {
            if (materialIndex == m->removeIndex)
                materialIndex = 0;

            if (m->transferIndex.has_value() && m->transferIndex == materialIndex)
                materialIndex = m->removeIndex;
        }
    }

    if (const auto m = message.getIf<Message::TextureDeleted>())
    {
        for (auto& material : mMaterialArray)
        {
            if (m->removedIndex == material.baseColorTexIndex)
                material.baseColorTexIndex = DefaultBaseColorTexIndex;

            if (m->removedIndex == material.metallicRoughnessTexIndex)
                material.metallicRoughnessTexIndex = DefaultMetallicRoughnessTexIndex;

            if (m->removedIndex == material.normalTexIndex)
                material.normalTexIndex = DefaultNormalTexIndex;

            if (m->removedIndex == material.aoTexIndex)
                material.aoTexIndex = DefaultAoTexIndex;

            if (m->removedIndex == material.emissionTexIndex)
                material.emissionTexIndex = DefaultEmissionTexIndex;

            if (m->transferIndex.has_value() && m->transferIndex == material.baseColorTexIndex)
                material.baseColorTexIndex = m->removedIndex;

            if (m->transferIndex.has_value() && m->transferIndex == material.metallicRoughnessTexIndex)
                material.metallicRoughnessTexIndex = m->removedIndex;

            if (m->transferIndex.has_value() && m->transferIndex == material.normalTexIndex)
                material.normalTexIndex = m->removedIndex;

            if (m->transferIndex.has_value() && m->transferIndex == material.aoTexIndex)
                material.aoTexIndex = m->removedIndex;

            if (m->transferIndex.has_value() && m->transferIndex == material.emissionTexIndex)
                material.emissionTexIndex = m->removedIndex;
        }

        mMaterialsSSBO.update(0, mMaterialArray.size() * sizeof(Material), mMaterialArray.data());
    }
}

void ResourceManager::processMainThreadTasks()
{
    while (auto task = mTaskQueue.pop())
        (*task)();

    for (auto itr = mLoadedModelFutures.begin(); itr != mLoadedModelFutures.end();)
    {
        if (itr->wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            auto loadedModel = itr->get();
            addModel(loadedModel);
            itr = mLoadedModelFutures.erase(itr);
        }
        else
            ++itr;
    }
}

std::shared_ptr<Model> ResourceManager::getModel(uuid64_t id)
{
    return mModels.at(id);
}

std::shared_ptr<InstancedMesh> ResourceManager::getMesh(uuid64_t id)
{
    return mMeshes.at(id);
}

std::shared_ptr<VulkanTexture> ResourceManager::getTexture(uuid64_t id)
{
    return mTextures.at(id);
}

std::shared_ptr<VulkanTexture> ResourceManager::getTextureFromIndex(index_t texIndex)
{
    return mMaterialTextureArray.at(texIndex);
}

index_t ResourceManager::getMatIndex(uuid64_t id)
{
    return mMaterials.at(id);
}

index_t ResourceManager::getTextureIndex(uuid64_t id)
{
    return mTextureIdToIndex.at(id);
}

uuid64_t ResourceManager::getTexIDFromIndex(index_t texIndex)
{
    auto texture = mMaterialTextureArray.at(texIndex);
    return getTID(mTextures, texture).value();
}

void ResourceManager::updateMaterial(index_t materialIndex)
{
    mMaterialsSSBO.update(materialIndex * sizeof(Material), sizeof(Material), &mMaterialArray.at(materialIndex));
}

void ResourceManager::addModel(std::shared_ptr<LoadedModelData> modelData)
{
    // add meshes, textures, and materials to resources
    std::unordered_map<index_t, uuid64_t> loadedMeshIndexToMeshUUID = addMeshes(modelData);
    std::unordered_map<index_t, uint32_t> loadedTextureIndexToResourceIndex = addTextures(modelData);
    std::unordered_map<std::string, uuid64_t> loadedMatNameToMatID = addMaterials(modelData, loadedTextureIndexToResourceIndex);

    // add model
    uuid64_t modelID = UUIDRegistry::generateModelID();
    std::shared_ptr<Model> model = std::make_shared<Model>();

    model->root = createModelNodeHierarchy(modelData, modelData->root, loadedMeshIndexToMeshUUID, loadedMatNameToMatID);
    model->bb = modelData->bb;
    model->mappedMaterials = loadedMatNameToMatID;

    mModels.emplace(modelID, model);
    mModelNames.emplace(modelID, modelData->name);
    mModelPaths.emplace(modelID, modelData->path);
}

std::unordered_map<index_t, uuid64_t> ResourceManager::addMeshes(std::shared_ptr<LoadedModelData> modelData)
{
    std::unordered_map<index_t, uuid64_t> loadedMeshIndexToMeshUUID;

    for (size_t i = 0; i < modelData->meshes.size(); ++i)
    {
        const auto& loadedMesh = modelData->meshes.at(i);

        uuid64_t meshID = UUIDRegistry::generateMeshID();
        mMeshes.emplace(meshID, loadedMesh.mesh);
        mMeshNames.emplace(meshID, loadedMesh.name);

        loadedMeshIndexToMeshUUID.emplace(i, meshID);
    }

    return loadedMeshIndexToMeshUUID;
}

std::unordered_map<index_t, uint32_t> ResourceManager::addTextures(std::shared_ptr<LoadedModelData> modelData)
{
    std::unordered_map<index_t, uint32_t> loadedTextureIndexToResourceIndex;

    for (index_t i = 0; i < modelData->textures.size(); ++i)
    {
        const auto& [texture, texturePath] = modelData->textures.at(i);

        uuid64_t textureID = UUIDRegistry::generateTexture2dID();
        std::string textureName = texturePath.filename().string();

        mTextures.emplace(textureID, texture);
        mTextureNames.emplace(textureID, textureName);
        mTexturePaths.emplace(textureID, texturePath);

        // create descriptor set
        mTexture2dDescriptorSets.emplace(textureID, createTextureDescriptorSet(texture, textureName));

        // add to material textures
        index_t insertedIndex = mMaterialTextureArray.size();

        mTextureIdToIndex.emplace(textureID, insertedIndex);
        mMaterialTextureArray.push_back(texture);

        // update mMaterialTexturesDescriptorSet
        VkDescriptorImageInfo imageInfo {
            .sampler = texture->vulkanSampler.sampler,
            .imageView = texture->vulkanImage.imageView,
            .imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
        };

        VkWriteDescriptorSet writeDescriptorSet {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = mMaterialTexturesDescriptorSet,
            .dstBinding = 0,
            .dstArrayElement = insertedIndex,
            .descriptorCount = 1,
            .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
            .pImageInfo = &imageInfo
        };

        vkUpdateDescriptorSets(mRenderDevice->device, 1, &writeDescriptorSet, 0, nullptr);

        // add to loadedTextureIndexToResourceIndex
        loadedTextureIndexToResourceIndex.emplace(i, insertedIndex);
    }

    return loadedTextureIndexToResourceIndex;
}

std::unordered_map<std::string, uuid64_t> ResourceManager::addMaterials(std::shared_ptr<LoadedModelData> modelData,
                                                                        const std::unordered_map<index_t, uint32_t> &loadedTextureIndexToResourceIndex)
{
    std::unordered_map<std::string, uuid64_t> loadedMatNameToMatID;

    for (size_t i = 0; i < modelData->materials.size(); ++i)
    {
        const auto& loadedMaterial = modelData->materials.at(i);

        Material material {
            .baseColorTexIndex = DefaultBaseColorTexIndex,
            .metallicRoughnessTexIndex = DefaultMetallicRoughnessTexIndex,
            .normalTexIndex = DefaultNormalTexIndex,
            .aoTexIndex = DefaultNormalTexIndex,
            .emissionTexIndex = DefaultEmissionTexIndex,
            .baseColorFactor = loadedMaterial.baseColorFactor,
            .emissionFactor = loadedMaterial.emissionColorFactor,
            .metallicFactor = loadedMaterial.metallicFactor,
            .roughnessFactor = loadedMaterial.roughnessFactor,
            .occlusionFactor = loadedMaterial.occlusionFactor,
            .tiling = glm::vec2(1.f),
            .offset = glm::vec2(0.f)
        };

        if (loadedMaterial.baseColorTexIndex != -1)
        {
            material.baseColorTexIndex = loadedTextureIndexToResourceIndex.at(modelData->getTextureIndex(loadedMaterial.baseColorTexIndex));
        }

        if (loadedMaterial.metallicRoughnessTexIndex != -1)
        {
            material.metallicRoughnessTexIndex = loadedTextureIndexToResourceIndex.at(modelData->getTextureIndex(loadedMaterial.metallicRoughnessTexIndex));
        }

        if (loadedMaterial.normalTexIndex != -1)
        {
            material.normalTexIndex = loadedTextureIndexToResourceIndex.at(modelData->getTextureIndex(loadedMaterial.normalTexIndex));
        }

        if (loadedMaterial.aoTexIndex != -1)
        {
            material.aoTexIndex = loadedTextureIndexToResourceIndex.at(modelData->getTextureIndex(loadedMaterial.aoTexIndex));
        }

        if (loadedMaterial.emissionTexIndex != -1)
        {
            material.emissionTexIndex = loadedTextureIndexToResourceIndex.at(modelData->getTextureIndex(loadedMaterial.emissionTexIndex));
        }

        uuid64_t materialID = UUIDRegistry::generateMaterialID();
        mMaterials.emplace(materialID, mMaterialArray.size());
        mMaterialNames.emplace(materialID, loadedMaterial.name);
        mMaterialArray.push_back(material);

        loadedMatNameToMatID.emplace(loadedMaterial.name, materialID);
    }

    mMaterialsSSBO.update(0, mMaterialArray.size() * sizeof(Material), mMaterialArray.data());

    return loadedMatNameToMatID;
}

Model::Node ResourceManager::createModelNodeHierarchy(std::shared_ptr<LoadedModelData> modelData,
                                                      const LoadedModelData::Node& loadedNode,
                                                      const std::unordered_map<index_t, uuid64_t>& loadedMeshIndexToMeshUUID,
                                                      const std::unordered_map<std::string, uuid64_t>& loadedMatNameToMatID)
{
    Model::Node node {
        .name = loadedNode.name,
        .transformation = loadedNode.transformation
    };

    if (auto meshIndex = loadedNode.meshIndex)
    {
        node.meshID = loadedMeshIndexToMeshUUID.at(*meshIndex);

        if (auto matIndex = modelData->meshes.at(*meshIndex).materialIndex)
        {
            node.materialName = modelData->materials.at(*matIndex).name;
        }
    }

    for (const auto& child : loadedNode.children)
        node.children.push_back(
            createModelNodeHierarchy(modelData, child, loadedMeshIndexToMeshUUID, loadedMatNameToMatID));

    return node;
}

void ResourceManager::deleteModel(uuid64_t id)
{
    std::shared_ptr<Model> model = mModels.at(id);

    // delete model
    mModels.erase(id);
    mModelNames.erase(id);
    mModelPaths.erase(id);

    // delete model meshes
    std::unordered_set<uuid64_t> meshIDs = getModelMeshIDs(*model);

    for (uuid64_t meshID : meshIDs)
    {
        mMeshes.erase(meshID);
        mMeshNames.erase(meshID);
    }

    // send message
    SNS::publishMessage(Topic::Type::Resources, Message::create<Message::ModelDeleted>(id, meshIDs));
}

void ResourceManager::deleteTexture(uuid64_t id)
{
    std::shared_ptr<VulkanTexture> texture = mTextures.at(id);

    // remove texture from map
    mTextures.erase(id);
    mTextureNames.erase(id);
    mTexturePaths.erase(id);
    mTexture2dDescriptorSets.erase(id);

    // remove texture from mMaterialTextureArray
    index_t removeIndex = mTextureIdToIndex.at(id);

    std::optional<index_t> transferIndex;
    if (removeIndex != mMaterialTextureArray.size() - 1)
    {
        index_t lastIndex = mMaterialTextureArray.size() - 1;
        std::swap(mMaterialTextureArray.at(removeIndex), mMaterialTextureArray.at(lastIndex));
        transferIndex = lastIndex;
    }

    mMaterialTextureArray.pop_back();

    recreateMaterialTextureDS();

    // send message
    SNS::publishMessage(Topic::Type::SceneGraph, Message::create<Message::TextureDeleted>(id, removeIndex, transferIndex));
}

void ResourceManager::deleteMaterial(uuid64_t id)
{
    index_t removeIndex = mMaterials.at(id);

    // delete material
    mMaterials.erase(id);
    mMaterialNames.erase(id);

    // reformat material array
    std::optional<index_t> transferIndex;
    if (removeIndex != mMaterialArray.size() - 1)
    {
        index_t lastIndex = mMaterialArray.size() - 1;
        std::swap(mMaterialArray.at(lastIndex), mMaterialArray.at(removeIndex));
        transferIndex = lastIndex;
    }

    mMaterialArray.pop_back();

    // update SSBO
    mMaterialsSSBO.update(0, mMaterialArray.size() * sizeof(Material), mMaterialArray.data());

    // send message
    SNS::publishMessage(Topic::Type::Resources, Message::create<Message::MaterialDeleted>(id, removeIndex, transferIndex));
}

void ResourceManager::loadDefaultTextures()
{
    TextureSpecification textureSpecification {
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .width = 1,
        .height = 1,
        .layerCount = 1,
        .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
        .imageUsage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageAspect = VK_IMAGE_ASPECT_COLOR_BIT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .wrapMode = TextureWrap::Repeat,
        .filterMode = TextureFilter::Nearest,
        .generateMipMaps = false
    };

    float baseColorTexData[4] {1.f, 1.f, 1.f, 1.f};
    float metallicRoughnessTexData[4] {1.f, 1.f, 1.f, 1.f};
    float normalTexData[4] {0.5f, 0.5f, 1.f, 0.f};
    float aoTexData[4] {1.f, 1.f, 1.f, 1.f};
    float emissionTexData[4] {0.f, 0.f, 0.f, 0.f};

    const char* baseColorTexName = "Default Base Color";
    const char* metallicRoughnessTexName = "Default Metallic Roughness";
    const char* normalTexName = "Default Normal";
    const char* aoTexName = "Default Ambient Occlusion";
    const char* emissionTexName = "Default Emission";

    auto baseColorTex = std::make_shared<VulkanTexture>(*mRenderDevice, textureSpecification, baseColorTexData);
    auto metallicRoughnessTex = std::make_shared<VulkanTexture>(*mRenderDevice, textureSpecification, metallicRoughnessTexData);
    auto normalTex = std::make_shared<VulkanTexture>(*mRenderDevice, textureSpecification, normalTexData);
    auto aoTex = std::make_shared<VulkanTexture>(*mRenderDevice, textureSpecification, aoTexData);
    auto emissionTex = std::make_shared<VulkanTexture>(*mRenderDevice, textureSpecification, emissionTexData);

    baseColorTex->vulkanImage.transitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    metallicRoughnessTex->vulkanImage.transitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    normalTex->vulkanImage.transitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    aoTex->vulkanImage.transitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    emissionTex->vulkanImage.transitionLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    uuid64_t baseColorID = UUIDRegistry::getDefTexID(MatTexType::BaseColor);
    uuid64_t metallicRoughnessID = UUIDRegistry::getDefTexID(MatTexType::MetallicRoughness);
    uuid64_t normalID = UUIDRegistry::getDefTexID(MatTexType::Normal);
    uuid64_t aoID = UUIDRegistry::getDefTexID(MatTexType::Ao);
    uuid64_t emissionID = UUIDRegistry::getDefTexID(MatTexType::Emission);

    mTextures.emplace(baseColorID, baseColorTex);
    mTextures.emplace(metallicRoughnessID, metallicRoughnessTex);
    mTextures.emplace(normalID, normalTex);
    mTextures.emplace(aoID, aoTex);
    mTextures.emplace(emissionID, emissionTex);

    mTextureNames.emplace(baseColorID, baseColorTexName);
    mTextureNames.emplace(metallicRoughnessID, metallicRoughnessTexName);
    mTextureNames.emplace(normalID, normalTexName);
    mTextureNames.emplace(aoID, aoTexName);
    mTextureNames.emplace(emissionID, emissionTexName);

    mTextureIdToIndex.emplace(baseColorID, DefaultBaseColorTexIndex);
    mTextureIdToIndex.emplace(metallicRoughnessID, DefaultMetallicRoughnessTexIndex);
    mTextureIdToIndex.emplace(normalID, DefaultNormalTexIndex);
    mTextureIdToIndex.emplace(aoID, DefaultAoTexIndex);
    mTextureIdToIndex.emplace(emissionID, DefaultEmissionTexIndex);

    mTexture2dDescriptorSets.emplace(baseColorID, createTextureDescriptorSet(baseColorTex, baseColorTexName));
    mTexture2dDescriptorSets.emplace(metallicRoughnessID, createTextureDescriptorSet(metallicRoughnessTex, metallicRoughnessTexName));
    mTexture2dDescriptorSets.emplace(normalID, createTextureDescriptorSet(normalTex, normalTexName));
    mTexture2dDescriptorSets.emplace(aoID, createTextureDescriptorSet(aoTex, aoTexName));
    mTexture2dDescriptorSets.emplace(emissionID, createTextureDescriptorSet(emissionTex, emissionTexName));

    mMaterialTextureArray.push_back(baseColorTex);
    mMaterialTextureArray.push_back(metallicRoughnessTex);
    mMaterialTextureArray.push_back(normalTex);
    mMaterialTextureArray.push_back(aoTex);
    mMaterialTextureArray.push_back(emissionTex);

    recreateMaterialTextureDS();
}

void ResourceManager::loadDefaultMaterial()
{
    Material material {
        .baseColorTexIndex = DefaultBaseColorTexIndex,
        .metallicRoughnessTexIndex = DefaultMetallicRoughnessTexIndex,
        .normalTexIndex = DefaultNormalTexIndex,
        .aoTexIndex = DefaultAoTexIndex,
        .emissionTexIndex = DefaultEmissionTexIndex,
        .baseColorFactor = glm::vec4(0.5f, 0.5f, 0.5f, 1.f),
        .emissionFactor = glm::vec4(0.f),
        .metallicFactor = 1.f,
        .roughnessFactor = 1.f,
        .occlusionFactor = 1.f,
        .tiling = glm::vec2(1.f),
        .offset = glm::vec2(0.f)
    };

    uuid64_t materialID = UUIDRegistry::getDefMatID();
    mMaterials.emplace(materialID, 0);
    mMaterialNames.emplace(materialID, "Default Material");
    mMaterialArray.push_back(material);

    mMaterialsSSBO.update(0, sizeof(Material), mMaterialArray.data());
}

void ResourceManager::recreateMaterialTextureDS()
{
    uint32_t textureCount = mMaterialTextureArray.size();

    std::vector<VkDescriptorImageInfo> imageInfos(textureCount);
    for (index_t i = 0; i < textureCount; ++i)
    {
        auto texture = mMaterialTextureArray.at(i);

        imageInfos.at(i).sampler = texture->vulkanSampler.sampler;
        imageInfos.at(i).imageView = texture->vulkanImage.imageView;
        imageInfos.at(i).imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    }

    VkWriteDescriptorSet writeDescriptorSet {
        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
        .dstSet = mMaterialTexturesDescriptorSet,
        .dstBinding = 0,
        .dstArrayElement = 0,
        .descriptorCount = textureCount,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .pImageInfo = imageInfos.data()
    };

    vkUpdateDescriptorSets(mRenderDevice->device, 1, &writeDescriptorSet, 0, nullptr);
}

VkDescriptorSet ResourceManager::createTextureDescriptorSet(const std::shared_ptr<VulkanTexture> texture, const std::string& name)
{
    VkDescriptorSet textureDS = mDSBuilder
        .setLayout(mTexture2dDescriptorSetLayout)
        .addTexture(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 0, *texture)
        .setDebugName(name)
        .create();

    return textureDS;
}

VkDescriptorSet ResourceManager::getTextureDescriptorSet(uuid64_t id)
{
    return mTexture2dDescriptorSets.at(id);
}

void ResourceManager::createDescriptorResources()
{
    mTexture2dDescriptorSetLayout = mDSLayoutBuilder
        .addLayoutBinding(0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT)
        .setDebugName("ResourceManager::mTexture2dDescriptorSetLayout")
        .create();

    uint32_t maxTextureCount = 256;
    VkDescriptorSetLayoutBinding descriptorSetLayoutBinding {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = maxTextureCount,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };

    VkDescriptorBindingFlagsEXT descriptorBindingFlags = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT;
    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT descriptorSetLayoutBindingFlagsCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount = 1,
        .pBindingFlags = &descriptorBindingFlags
    };

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &descriptorSetLayoutBindingFlagsCreateInfo,
        .bindingCount = 1,
        .pBindings = &descriptorSetLayoutBinding
    };

    VkResult result = vkCreateDescriptorSetLayout(mRenderDevice->device,
                                                   &descriptorSetLayoutCreateInfo,
                                                   nullptr,
                                                   &mMaterialTexturesDescriptorSetLayout);
    vulkanCheck(result, "Failed to create mTexture2dDescriptorSetLayout.");

    setVulkanObjectDebugName(*mRenderDevice,
                             VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT,
                             "ResourceManager::mTexture2dDescriptorSetLayout",
                             mMaterialTexturesDescriptorSetLayout);

    VkDescriptorSetVariableDescriptorCountAllocateInfoEXT variableDescriptorCountAllocInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
        .descriptorSetCount = 1,
        .pDescriptorCounts = &maxTextureCount
    };

    VkDescriptorSetAllocateInfo descriptorSetAllocateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = &variableDescriptorCountAllocInfo,
        .descriptorPool = mRenderDevice->descriptorPool,
        .descriptorSetCount = 1,
        .pSetLayouts = &mMaterialTexturesDescriptorSetLayout
    };

    result = vkAllocateDescriptorSets(mRenderDevice->device, &descriptorSetAllocateInfo, &mMaterialTexturesDescriptorSet);
    vulkanCheck(result, "Failed to create mMaterialTexturesDescriptorSet.");

    setVulkanObjectDebugName(*mRenderDevice,
                             VK_OBJECT_TYPE_DESCRIPTOR_SET,
                             "ResourceManager::mMaterialTexturesDescriptorSet",
                             mMaterialTexturesDescriptorSet);
}

uuid64_t ResourceManager::getMatIdFromIndex(index_t matIndex)
{
    return getTID(mMaterials, matIndex).value();
}
