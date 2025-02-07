//
// Created by Gianni on 23/01/2025.
//

#ifndef VULKANRENDERINGENGINE_RESOURCE_MANAGER_HPP
#define VULKANRENDERINGENGINE_RESOURCE_MANAGER_HPP

#include "../app/simple_notification_service.hpp"
#include "../app/uuid_registry.hpp"
#include "../renderer/material.hpp"
#include "../utils/main_thread_task_queue.hpp"
#include "../vk/vulkan_descriptor.hpp"
#include "resource_importer.hpp"

class Editor;
class Renderer;

class ResourceManager : public SubscriberSNS
{
public:
    ResourceManager(const std::shared_ptr<VulkanRenderDevice> renderDevice);
    ~ResourceManager();

    bool importModel(const std::filesystem::path& path);

    void notify(const Message &message) override;

    void processMainThreadTasks();

    std::shared_ptr<Model> getModel(uuid64_t id);
    std::shared_ptr<InstancedMesh> getMesh(uuid64_t id);
    std::shared_ptr<VulkanTexture> getTexture(uuid64_t id);
    std::shared_ptr<VulkanTexture> getTextureFromIndex(index_t texIndex);
    index_t getMatIndex(uuid64_t id);
    index_t getTextureIndex(uuid64_t id);
    uuid64_t getTexIDFromIndex(index_t texIndex);
    uuid64_t getMatIdFromIndex(index_t matIndex);
    VkDescriptorSet getTextureDescriptorSet(uuid64_t id);

    void updateMaterial(index_t materialIndex);

    void deleteModel(uuid64_t id);
    void deleteTexture(uuid64_t id);
    void deleteMaterial(uuid64_t id);

private:
    void createDescriptorResources();
    void loadDefaultTextures();
    void loadDefaultMaterial();
    void recreateMaterialTextureDS();

    void addModel(std::shared_ptr<LoadedModelData> modelData);
    std::unordered_map<index_t, uuid64_t> addMeshes(std::shared_ptr<LoadedModelData> modelData);
    std::unordered_map<index_t, uint32_t> addTextures(std::shared_ptr<LoadedModelData> modelData);
    std::unordered_map<std::string, uuid64_t> addMaterials(std::shared_ptr<LoadedModelData> modelData,
                                                           const std::unordered_map<index_t, uint32_t>& loadedTextureIndexToResourceIndex);
    Model::Node createModelNodeHierarchy(std::shared_ptr<LoadedModelData> modelData,
                                         const LoadedModelData::Node& loadedNode,
                                         const std::unordered_map<index_t, uuid64_t>& loadedMeshIndexToMeshUUID,
                                         const std::unordered_map<std::string, uuid64_t>& loadedMatNameToMatID);

    VkDescriptorSet createTextureDescriptorSet(const std::shared_ptr<VulkanTexture> texture, const std::string& name);

private:
    // All models
    std::unordered_map<uuid64_t, std::shared_ptr<Model>> mModels;
    std::unordered_map<uuid64_t, std::string> mModelNames;
    std::unordered_map<uuid64_t, std::filesystem::path> mModelPaths;

    // All meshes
    std::unordered_map<uuid64_t, std::shared_ptr<InstancedMesh>> mMeshes;
    std::unordered_map<uuid64_t, std::string> mMeshNames;

    // All textures
    std::unordered_map<uuid64_t, std::shared_ptr<VulkanTexture>> mTextures;
    std::unordered_map<uuid64_t, std::string> mTextureNames;
    std::unordered_map<uuid64_t, std::filesystem::path> mTexturePaths;
    std::unordered_map<uuid64_t, index_t> mTextureIdToIndex;
    std::unordered_map<uuid64_t, VkDescriptorSet> mTexture2dDescriptorSets;
    std::unordered_map<uuid64_t, std::array<VkDescriptorSet, 6>> mTextureCubeDescriptorSets;
    VkDescriptorSetLayout mTexture2dDescriptorSetLayout;

    std::vector<std::shared_ptr<VulkanTexture>> mMaterialTextureArray;
    VkDescriptorSetLayout mMaterialTexturesDescriptorSetLayout;
    VkDescriptorSet mMaterialTexturesDescriptorSet; // The order of the descriptors will need to match the texture array

    // All materials
    std::unordered_map<uuid64_t, index_t> mMaterials;
    std::map<uuid64_t, std::string> mMaterialNames;
    std::vector<Material> mMaterialArray;
    VulkanBuffer mMaterialsSSBO;

    // Async Loading
    std::vector<std::future<std::shared_ptr<LoadedModelData>>> mLoadedModelFutures;
    MainThreadTaskQueue mTaskQueue;

    // descriptor builders
    DescriptorSetLayoutBuilder mDSLayoutBuilder;
    DescriptorSetBuilder mDSBuilder;

    const std::shared_ptr<VulkanRenderDevice> mRenderDevice;

private:
    friend class Editor;
    friend class Renderer;
};

#endif //VULKANRENDERINGENGINE_RESOURCE_MANAGER_HPP
