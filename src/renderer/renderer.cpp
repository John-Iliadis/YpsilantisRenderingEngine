//
// Created by Gianni on 4/01/2025.
//

#include "renderer.hpp"

Renderer::Renderer(const VulkanRenderDevice& renderDevice)
    : mRenderDevice(renderDevice)
{
    createDisplayTexturesDsLayout();
    createMaterialDsLayout();
}

Renderer::~Renderer()
{
    vkDestroyDescriptorSetLayout(mRenderDevice.device, mDisplayTexturesDSLayout, nullptr);
    vkDestroyDescriptorSetLayout(mRenderDevice.device, mMaterialsDsLayout, nullptr);
}

void Renderer::importModel(const std::filesystem::path &path)
{
    bool loaded = std::find_if(mModels.begin(), mModels.end(), [&path] (const auto& pair) {
        return path == pair.second->path;
    }) != mModels.end();

    if (loaded)
    {
        debugLog("Model is already loaded.");
        return;
    }

    EnqueueCallback callback = [this] (Task&& t) { mTaskQueue.push(std::move(t)); };
    mLoadedModelFutures.push_back(ModelImporter::loadModel(path, &mRenderDevice, callback));
}

void Renderer::deleteModel(uuid32_t id)
{
    SNS::publishMessage(Topic::Type::Renderer, Message::modelDeleted(id));
    mModels.erase(id);
}

void Renderer::processMainThreadTasks()
{
    while (auto task = mTaskQueue.pop())
        (*task)();

    for (auto itr = mLoadedModelFutures.begin(); itr != mLoadedModelFutures.end();)
    {
        if (itr->wait_for(std::chrono::seconds(0)) == std::future_status::ready)
        {
            auto model = itr->get();

            uuid32_t modelID = UUIDRegistry::generateModelID();

            mModels.emplace(modelID, model);

            model->id = modelID;
            model->createMaterialsUBO(mRenderDevice);
            model->createTextureDescriptorSets(mRenderDevice, mDisplayTexturesDSLayout);
            model->createMaterialsDescriptorSet(mRenderDevice, mMaterialsDsLayout);

            itr = mLoadedModelFutures.erase(itr);
        }
        else
            ++itr;
    }
}

void Renderer::createDisplayTexturesDsLayout()
{
    VkDescriptorSetLayoutBinding dsLayoutBinding {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = 1,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };

    VkDescriptorSetLayoutCreateInfo dsLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .bindingCount = 1,
        .pBindings = &dsLayoutBinding
    };

    VkResult result = vkCreateDescriptorSetLayout(mRenderDevice.device,
                                                  &dsLayoutCreateInfo,
                                                  nullptr,
                                                  &mDisplayTexturesDSLayout);
    vulkanCheck(result, "Failed to create Renderer::mDisplayTexturesDSLayout.");
    setDsLayoutDebugName(mRenderDevice, mDisplayTexturesDSLayout, "Renderer::mDisplayTexturesDSLayout");
}

void Renderer::createMaterialDsLayout()
{
    VkDescriptorSetLayoutBinding materialsBinding {
        .binding = 0,
        .descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        .descriptorCount = PerModelMaxMaterialCount,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };

    VkDescriptorSetLayoutBinding texturesBinding {
        .binding = 1,
        .descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        .descriptorCount = PerModelMaxTextureCount,
        .stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT
    };

    VkDescriptorSetLayoutBinding bindings[2] {
        materialsBinding,
        texturesBinding
    };

    VkDescriptorBindingFlagsEXT descriptorBindingFlags[2] {
        0,
        VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT
    };

    VkDescriptorSetLayoutBindingFlagsCreateInfoEXT descriptorSetLayoutBindingFlagsCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount = 2,
        .pBindingFlags = descriptorBindingFlags
    };

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = &descriptorSetLayoutBindingFlagsCreateInfo,
        .bindingCount = 2,
        .pBindings = bindings
    };

    VkResult result = vkCreateDescriptorSetLayout(mRenderDevice.device,
                                                  &descriptorSetLayoutCreateInfo,
                                                  nullptr,
                                                  &mMaterialsDsLayout);
    vulkanCheck(result, "Failed to create Renderer::mMaterialsDsLayout.");
    setDsLayoutDebugName(mRenderDevice, mMaterialsDsLayout, "Renderer::mMaterialsDsLayout");
}
