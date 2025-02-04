//
// Created by Gianni on 14/01/2025.
//

#ifndef VULKANRENDERINGENGINE_MATERIAL_HPP
#define VULKANRENDERINGENGINE_MATERIAL_HPP

#include <glm/glm.hpp>

enum class MatTexType
{
    BaseColor,
    MetallicRoughness,
    Normal,
    Ao,
    Emission,
};

struct Material
{
    alignas(4) index_t baseColorTexIndex;
    alignas(4) index_t metallicRoughnessTexIndex;
    alignas(4) index_t normalTexIndex;
    alignas(4) index_t aoTexIndex;
    alignas(4) index_t emissionTexIndex;
    alignas(16) glm::vec4 baseColorFactor;
    alignas(16) glm::vec4 emissionFactor;
    alignas(4) float metallicFactor;
    alignas(4) float roughnessFactor;
    alignas(4) float occlusionFactor;
    alignas(8) glm::vec2 tiling;
    alignas(8) glm::vec2 offset;
};

inline constexpr index_t DefaultBaseColorTexIndex = 0;
inline constexpr index_t DefaultMetallicRoughnessTexIndex = 1;
inline constexpr index_t DefaultNormalTexIndex = 2;
inline constexpr index_t DefaultAoTexIndex = 3;
inline constexpr index_t DefaultEmissionTexIndex = 4;

#endif //VULKANRENDERINGENGINE_MATERIAL_HPP
