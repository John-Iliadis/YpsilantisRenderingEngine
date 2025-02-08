//
// Created by Gianni on 26/01/2025.
//

#ifndef OPENGLRENDERINGENGINE_LIGHT_NODE_HPP
#define OPENGLRENDERINGENGINE_LIGHT_NODE_HPP

#include "graph_node.hpp"

enum ShadowOption : uint32_t
{
    ShadowOptionNoShadows = 0,
    ShadowOptionSoftShadows = 1,
    ShadowOptionHardShadows = 2,
    ShadowOptionCount
};

enum ShadowSoftness : uint32_t
{
    ShadowSoftnessLow = 0,
    ShadowSoftnessMedium = 1,
    ShadowSoftnessHigh = 2,
    ShadowSoftnessCount
};

const char* toStr(ShadowOption shadowOption);
const char* toStr(ShadowSoftness softShadow);

struct LightSpecification
{
    glm::vec3 color;
    float intensity;
    ShadowOption shadowOption;
    ShadowSoftness shadowSoftness;
    float shadowStrength;
    float shadowBias;
};

struct LightBase : public GraphNode
{
    LightBase();
    LightBase(const LightSpecification& specification, NodeType type, const std::string& name);

    glm::vec3 color;
    float intensity;
    ShadowOption shadowOption;
    ShadowSoftness shadowSoftness;
    float shadowStrength;
    float shadowBias;
};

struct DirectionalLight : public LightBase
{
    DirectionalLight() = default;
    DirectionalLight(const LightSpecification& specification);
};

struct PointLight : public LightBase
{
    PointLight();
    PointLight(const LightSpecification& specification, float range = 10.f);

    std::array<glm::mat4, 6> views; // todo: create
    glm::mat4 projection;
    float range;
};

struct SpotLight : public LightBase
{
    SpotLight();
    SpotLight(const LightSpecification& specification, float range = 10.f);

    glm::mat4 projection;
    float range;
};

#endif //OPENGLRENDERINGENGINE_LIGHT_NODE_HPP
