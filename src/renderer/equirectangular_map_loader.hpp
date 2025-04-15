//
// Created by Gianni on 24/02/2025.
//

#ifndef VULKANRENDERINGENGINE_EQUIRECTANGULAR_MAP_LOADER_HPP
#define VULKANRENDERINGENGINE_EQUIRECTANGULAR_MAP_LOADER_HPP

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <stb/stb_image.h>
#include "../vk/vulkan_texture.hpp"
#include "../vk/vulkan_descriptor.hpp"
#include "../vk/vulkan_pipeline.hpp"

class EquirectangularMapLoader
{
public:
    EquirectangularMapLoader(const VulkanRenderDevice& renderDevice, const std::string& path);
    ~EquirectangularMapLoader();

    bool success() const;

    void get(VulkanTexture& outTexture);

private:
    void createEquirectangularMap(float* data);
    void createUBO();
    void createTarget();
    void createDescriptors();
    void createRenderpass();
    void createFramebuffer();
    void createPipeline();
    void execute();

private:
    const VulkanRenderDevice& mRenderDevice;
    bool mSuccess;

    VulkanBuffer mUBO;
    VulkanTexture mEquirectangularMap;
    VulkanTexture mTarget;

    int32_t mWidth;
    int32_t mHeight;
    int32_t mCubemapSize;

    VkRenderPass mRenderpass;
    VkFramebuffer mFramebuffer;
    VulkanGraphicsPipeline mPipeline;
    VulkanDsLayout mDsLayout;
    VkDescriptorSet mDs;
};

#endif //VULKANRENDERINGENGINE_EQUIRECTANGULAR_MAP_LOADER_HPP
