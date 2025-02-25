//
// Created by Gianni on 24/02/2025.
//

#ifndef VULKANRENDERINGENGINE_CUBEMAP_LOADER_HPP
#define VULKANRENDERINGENGINE_CUBEMAP_LOADER_HPP

#include "stb/stb_image.h"
#include "../../vk/vulkan_texture.hpp"
#include "../../vk/vulkan_buffer.hpp"

class CubemapLoader
{
public:
    CubemapLoader(const VulkanRenderDevice& renderDevice, const std::array<std::string, 6>& paths);
    ~CubemapLoader();

    bool success();

    void get(VulkanTexture& outTexture);

private:
    bool validate();
    bool loadData();
    void createTexture();

private:
    const VulkanRenderDevice& mRenderDevice;

    std::array<std::string, 6> mPaths;
    std::array<std::future<uint8_t*>, 6> mDataFutures;
    std::array<uint8_t*, 6> mData;

    VulkanTexture mCubemapTexture;

    bool mSuccess;
    int32_t mWidth;
    int32_t mHeight;
};

#endif //VULKANRENDERINGENGINE_CUBEMAP_LOADER_HPP
