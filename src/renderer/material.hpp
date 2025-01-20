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

enum TextureType
{
    None = 0,
    Albedo,
    Roughness,
    Metallic,
    Normal,
    Displacement,
    Ao,
    Emission,
    Specular,
    Count
};

struct GpuMaterial
{
    Workflow workflow;
    uint32_t albedoMapIndex = 0;
    uint32_t roughnessMapIndex = 0;
    uint32_t metallicMapIndex = 0;
    uint32_t normalMapIndex = 0;
    uint32_t displacementMapIndex = 0;
    uint32_t aoMapIndex = 0;
    uint32_t emissionMapIndex = 0;
    uint32_t specularMapIndex = 0;
    glm::vec4 albedoColor = glm::vec4(1.f);
    glm::vec4 emissionColor = glm::vec4(0.f);
    glm::vec2 tiling = glm::vec2(1.f);
    glm::vec2 offset = glm::vec2(0.f);
};

struct NamedMaterial
{
    std::string name;
    GpuMaterial* material;
};

#endif //VULKANRENDERINGENGINE_MATERIAL_HPP
