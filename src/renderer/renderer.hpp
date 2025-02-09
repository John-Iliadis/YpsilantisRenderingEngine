//
// Created by Gianni on 4/01/2025.
//

#ifndef VULKANRENDERINGENGINE_RENDERER_HPP
#define VULKANRENDERINGENGINE_RENDERER_HPP

#include "../utils/utils.hpp"
#include "../utils/main_thread_task_queue.hpp"
#include "model_importer.hpp"
#include "model.hpp"

class Renderer
{
public:
    Renderer(const VulkanRenderDevice& renderDevice);
    ~Renderer();

    void importModel(const std::filesystem::path& path);
    void deleteModel(uuid32_t id);
    void processMainThreadTasks();

private:

    void createDisplayTexturesDsLayout();
    void createMaterialDsLayout();

private:
    const VulkanRenderDevice& mRenderDevice;

    VkDescriptorSetLayout mDisplayTexturesDSLayout;
    VkDescriptorSetLayout mMaterialsDsLayout;

    std::unordered_map<uuid32_t, std::shared_ptr<Model>> mModels;

    // async loading
    std::vector<std::future<std::shared_ptr<Model>>> mLoadedModelFutures;
    MainThreadTaskQueue mTaskQueue;

private:
    friend class Editor;
};

#endif //VULKANRENDERINGENGINE_RENDERER_HPP
