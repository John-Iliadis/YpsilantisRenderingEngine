//
// Created by Gianni on 28/02/2025.
//

#ifndef VULKANRENDERINGENGINE_LIGHTS_HPP
#define VULKANRENDERINGENGINE_LIGHTS_HPP

#include <glm/glm.hpp>

enum class LightType
{
    Directional,
    Point,
    Spot
};

struct DirectionalLight
{
    alignas(16) glm::vec4 color;
    alignas(16) glm::vec4 direction;
    alignas(4) float intensity;
};

struct PointLight
{
    alignas(16) glm::vec4 color;
    alignas(16) glm::vec4 position;
    alignas(4) float intensity;
    alignas(4) float range;
};

struct SpotLight
{
    alignas(16) glm::vec4 color;
    alignas(16) glm::vec4 position;
    alignas(16) glm::vec4 direction;
    alignas(4) float intensity;
    alignas(4) float range;
    alignas(4) float innerAngle; // rad
    alignas(4) float outerAngle; // rad
};

#endif //VULKANRENDERINGENGINE_LIGHTS_HPP
