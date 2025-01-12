//
// Created by Gianni on 11/01/2025.
//

#ifndef VULKANRENDERINGENGINE_SCENE_HPP
#define VULKANRENDERINGENGINE_SCENE_HPP

#include <glm/glm.hpp>
#include "mesh.hpp"

enum class ObjectType
{
    Mesh,
    DirectionalLight,
    PointLight,
    SpotLight
};

struct SceneObject
{
    uint32_t id;
    ObjectType type;
    std::string name;
    glm::mat4 localTransform;
    glm::mat4 globalTransform;
    bool dirty;
};

struct MeshObject : SceneObject
{
    uint32_t meshIndex;
    uint32_t materialIndex;
    uint32_t instanceID;
};

enum class ShadowOption : uint32_t
{
    NoShadows = 0,
    HardShadows = 1,
    SoftShadows = 2
};

enum class SoftShadow : uint32_t
{
    Low = 0,
    Medium = 1,
    High = 2
};

struct LightBase : SceneObject
{
    glm::vec3 color;
    float intensity;
    ShadowOption shadowOption;
    SoftShadow softShadow;
    float shadowStrength;
    float bias;
    uint32_t iconTextureIndex;
};

struct DirectionalLight : LightBase
{
    glm::vec3 direction;
    uint32_t shadowAtlasIndex;
    glm::vec2 shadowAtlasTexCoords;
};

struct PointLight : LightBase
{
    glm::vec3 position;
    std::array<glm::mat4, 6> views;
    glm::mat4 projection;
    float range;
    uint32_t cubemapIndex;
};

struct SpotLight : LightBase
{
    glm::vec3 position;
    glm::vec3 direction;
    glm::mat4 projection;
    float range;
    uint32_t shadowAtlasIndex;
    glm::vec2 shadowAtlasTexCoords;
};

#endif //VULKANRENDERINGENGINE_SCENE_HPP
