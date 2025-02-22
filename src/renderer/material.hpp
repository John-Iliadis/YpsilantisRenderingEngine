//
// Created by Gianni on 22/02/2025.
//

#ifndef VULKANRENDERINGENGINE_MATERIAL_HPP
#define VULKANRENDERINGENGINE_MATERIAL_HPP

#include "../vk/vulkan_texture.hpp"

struct alignas(64) Material
{
    alignas(4) int32_t baseColorTexIndex;
    alignas(4) int32_t metallicTexIndex;
    alignas(4) int32_t roughnessTexIndex;
    alignas(4) int32_t normalTexIndex;
    alignas(4) int32_t aoTexIndex;
    alignas(4) int32_t emissionTexIndex;
    alignas(4) float metallicFactor;
    alignas(4) float roughnessFactor;
    alignas(16) glm::vec4 baseColorFactor;
    alignas(16) glm::vec4 emissionFactor;
    alignas(8) glm::vec2 tiling;
    alignas(8) glm::vec2 offset;
};

bool hasTextures(const Material& material);

void createDefaultMaterialTextures(const VulkanRenderDevice& renderDevice);
void destroyDefaultMaterialTextures();

inline VulkanTexture defaultBaseColorTex;
inline VulkanTexture defaultMetallicTex;
inline VulkanTexture defaultRoughnessTex;
inline VulkanTexture defaultNormalTex;
inline VulkanTexture defaultAoTex;
inline VulkanTexture defaultEmissionTex;

#endif //VULKANRENDERINGENGINE_MATERIAL_HPP
