//
// Created by Gianni on 26/01/2025.
//

#include "light_node.hpp"

const char* toStr(ShadowOption shadowOption)
{
    switch (shadowOption)
    {
        case ShadowOptionNoShadows: return "No Shadows";
        case ShadowOptionSoftShadows: return "Soft Shadows";
        case ShadowOptionHardShadows: return "Hard Shadows";
        default: assert(false);
    }
}

const char* toStr(ShadowSoftness softShadow)
{
    switch (softShadow)
    {
        case ShadowSoftnessLow: return "Low";
        case ShadowSoftnessMedium: return "Medium";
        case ShadowSoftnessHigh: return "High";
        default: assert(false);
    }
}

LightBase::LightBase()
    : color()
    , intensity()
    , shadowOption()
    , shadowSoftness()
    , shadowStrength()
    , shadowBias()
{
}

LightBase::LightBase(const LightSpecification &specification, NodeType type, const std::string& name)
    : SceneNode(type, name, glm::identity<glm::mat4>(), nullptr)
    , color(specification.color)
    , intensity(specification.intensity)
    , shadowOption(specification.shadowOption)
    , shadowSoftness(specification.shadowSoftness)
    , shadowStrength(specification.shadowStrength)
    , shadowBias(specification.shadowBias)
{
}

DirectionalLight::DirectionalLight(const LightSpecification &specification)
    : LightBase(specification, NodeType::DirectionalLight, "Directional Light")
{
}

PointLight::PointLight()
    : range()
{
}

PointLight::PointLight(const LightSpecification &specification, float range)
    : LightBase(specification, NodeType::PointLight, "Point Light")
    , range(range)
{
}

SpotLight::SpotLight()
    : range()
{
}

SpotLight::SpotLight(const LightSpecification &specification, float range)
    : LightBase(specification, NodeType::SpotLight, "Spot Light")
    , range(range)
{
}
