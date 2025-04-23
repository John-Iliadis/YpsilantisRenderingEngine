//
// Created by Gianni on 28/02/2025.
//

#ifndef VULKANRENDERINGENGINE_LIGHTS_HPP
#define VULKANRENDERINGENGINE_LIGHTS_HPP

#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>

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
    alignas(4) float innerAngle;
    alignas(4) float outerAngle;
};

enum class ShadowType : uint32_t
{
    NoShadow,
    HardShadow,
    SoftShadow
};

struct PointShadowData
{
    glm::mat4 viewProj[6] {};
    ShadowType shadowType = ShadowType::HardShadow;
    uint32_t resolution = 1024;
    float strength = 1;
    float biasSlope = 0.2f;
    float biasConstant = 0.01f;
    float pcfRadius = 10.f;
    uint32_t padding[2] {};
};

struct PointShadowMap
{
    VulkanImage shadowMap;
    VkFramebuffer framebuffers[6];
};

struct SpotShadowData
{
    glm::mat4 viewProj {};
    ShadowType shadowType = ShadowType::HardShadow;
    uint32_t resolution = 1024;
    float strength = 1;
    float biasSlope = 0.f;
    float biasConstant = 0.0002f;
    int32_t pcfRange = 2;
    uint32_t padding[2] {};
};

struct SpotShadowMap
{
    VulkanImage shadowMap;
    VkFramebuffer framebuffer{};
};

inline const char* toStr(ShadowType shadowType)
{
    switch (shadowType)
    {

        case ShadowType::NoShadow: return "No Shadow";
        case ShadowType::HardShadow: return "Hard Shadow";
        case ShadowType::SoftShadow: return "Soft Shadow";
        default: return "Unknown";
    }
}

inline void calcMatrices(PointShadowData& shadowData, const PointLight& pointLight)
{
    glm::mat4 proj = glm::perspective(glm::half_pi<float>(), 1.f, 0.1f, pointLight.range);
    glm::vec3 lightPos(pointLight.position);

    shadowData.viewProj[0] = proj * glm::lookAt(lightPos, lightPos + glm::vec3( 1, 0, 0), glm::vec3(0, -1, 0)); // +X
    shadowData.viewProj[1] = proj * glm::lookAt(lightPos, lightPos + glm::vec3(-1, 0, 0), glm::vec3(0, -1, 0)); // -X
    shadowData.viewProj[2] = proj * glm::lookAt(lightPos, lightPos + glm::vec3(0, 1, 0), glm::vec3(0, 0, 1));   // +Y
    shadowData.viewProj[3] = proj * glm::lookAt(lightPos, lightPos + glm::vec3(0, -1, 0), glm::vec3(0, 0,-1));   // -Y
    shadowData.viewProj[4] = proj * glm::lookAt(lightPos, lightPos + glm::vec3(0, 0, 1), glm::vec3(0, -1, 0));  // +Z
    shadowData.viewProj[5] = proj * glm::lookAt(lightPos, lightPos + glm::vec3(0, 0,-1), glm::vec3(0, -1, 0));  // -Z
}

inline void calcMatrices(SpotShadowData& shadowData, const SpotLight& spotLight)
{
    glm::mat4 view = glm::lookAt(glm::vec3(spotLight.position),
                           glm::vec3(spotLight.position + spotLight.direction),
                           glm::vec3(0.f, 1.f, 0.f));
    glm::mat4 proj = glm::perspective(glm::radians(spotLight.outerAngle), 1.f, 0.1f, spotLight.range);
    proj[1][1] *= -1.f;

    shadowData.viewProj = proj * view;
}

inline glm::vec3 calcLightDir(const glm::vec3& angles)
{
    static constexpr glm::vec3 sInitialDir(0.f, 0.f, -1.f);
    glm::mat3 rotation = glm::toMat3(glm::quat(glm::radians(angles)));
    return rotation * sInitialDir;
}

#endif //VULKANRENDERINGENGINE_LIGHTS_HPP
