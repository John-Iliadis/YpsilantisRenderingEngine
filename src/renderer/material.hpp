//
// Created by Gianni on 14/01/2025.
//

#ifndef VULKANRENDERINGENGINE_MATERIAL_HPP
#define VULKANRENDERINGENGINE_MATERIAL_HPP

#include <glm/glm.hpp>

enum class Workflow : uint32_t
{
    Metallic = 1,
    Specular = 2
};

struct GpuMaterial
{
    Workflow workflow;
    uint32_t albedoMapIndex;
    uint32_t roughnessMapIndex;
    uint32_t metallicMapIndex;
    uint32_t normalMapIndex;
    uint32_t heightMapIndex;
    uint32_t aoMapIndex;
    uint32_t emissionMapIndex;
    uint32_t specularMapIndex;
    glm::vec4 albedoColor;
    glm::vec4 emissionColor;
    glm::vec2 tiling;
    glm::vec2 offset;
};

struct Material
{
    std::string name;
    GpuMaterial material;
};

#endif //VULKANRENDERINGENGINE_MATERIAL_HPP
