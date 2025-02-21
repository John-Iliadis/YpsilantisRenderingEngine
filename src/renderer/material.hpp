//
// Created by Gianni on 22/02/2025.
//

#ifndef VULKANRENDERINGENGINE_MATERIAL_HPP
#define VULKANRENDERINGENGINE_MATERIAL_HPP

struct Material
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

inline bool hasTextures(const Material& material)
{
    return material.baseColorTexIndex != -1 ||
        material.metallicTexIndex != -1 ||
        material.roughnessTexIndex != -1 ||
        material.normalTexIndex != -1 ||
        material.aoTexIndex != -1 ||
        material.emissionTexIndex != -1;
}

#endif //VULKANRENDERINGENGINE_MATERIAL_HPP
