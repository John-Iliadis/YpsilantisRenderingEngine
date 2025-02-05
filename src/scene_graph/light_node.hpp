//
// Created by Gianni on 26/01/2025.
//

#ifndef OPENGLRENDERINGENGINE_LIGHT_NODE_HPP
#define OPENGLRENDERINGENGINE_LIGHT_NODE_HPP

#include "scene_node.hpp"

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

class LightBase : public SceneNode
{
public:
    glm::vec3 color;
    float intensity;
    ShadowOption shadowOption;
    SoftShadow softShadow;
    float shadowStrength;
    float bias;
    uint32_t iconTextureIndex;

public:
};

class DirectionalLight : public LightBase
{
public:
    glm::vec3 direction;
    uint32_t shadowAtlasIndex;
    glm::vec2 shadowAtlasTexCoords;

public:
};

class PointLight : public LightBase
{
public:
    glm::vec3 position;
    std::array<glm::mat4, 6> views;
    glm::mat4 projection;
    float range;
    uint32_t cubemapIndex;

public:
};

class SpotLight : public LightBase
{
public:
    glm::vec3 position;
    glm::vec3 direction;
    glm::mat4 projection;
    float range;
    uint32_t shadowAtlasIndex;
    glm::vec2 shadowAtlasTexCoords;

public:
};

#endif //OPENGLRENDERINGENGINE_LIGHT_NODE_HPP
