//
// Created by Gianni on 13/01/2025.
//

#include "importer.hpp"

static std::mutex sLoadModelMutex;

static void loadModelAsync(std::vector<Model>& models,
                           const VulkanRenderDevice &renderDevice,
                           const std::string &path,
                           uint32_t importFlags)
{
    Model model;
    model.create(renderDevice, path, importFlags);

    std::lock_guard<std::mutex> lock(sLoadModelMutex);
    models.push_back(std::move(model));
}

void Importer::loadModel(std::vector<Model> &models,
                         const VulkanRenderDevice &renderDevice,
                         const std::string &path,
                         uint32_t importFlags)
{
    mLoadedModels.push_back(std::async(std::launch::async, loadModelAsync,
                                       std::ref(models), std::ref(renderDevice), std::ref(path), importFlags));
}
