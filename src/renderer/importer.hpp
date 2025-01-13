//
// Created by Gianni on 13/01/2025.
//

#ifndef VULKANRENDERINGENGINE_IMPORTER_HPP
#define VULKANRENDERINGENGINE_IMPORTER_HPP

#include "model.hpp"

class Importer
{
public:
    void loadModel(std::vector<Model>& models,
              const VulkanRenderDevice& renderDevice,
              const std::string& path,
              uint32_t importFlags);

private:
    std::vector<std::future<void>> mLoadedModels;
};

#endif //VULKANRENDERINGENGINE_IMPORTER_HPP
