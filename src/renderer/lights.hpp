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

inline constexpr uint32_t MaxCascades = 9;
struct alignas(16) DirShadowData
{
    glm::mat4 viewProj[MaxCascades] {};
    float cascadeDist[MaxCascades] {};
    ShadowType shadowType = ShadowType::HardShadow;
    uint32_t resolution = 1024;
    float strength = 1;
    float biasSlope = 0.f;
    float biasConstant = 0.01f;
    int32_t pcfRange = 3.f;
    uint32_t cascadeCount = 4;
    float zScalar = 10.f;
};

struct DirShadowMap
{
    VulkanImage shadowMap;
    std::vector<VkFramebuffer> framebuffers;
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

inline std::vector<glm::vec4> getFrustumCornersWS(const glm::mat4& viewProj)
{
    glm::mat4 inv = glm::inverse(viewProj);

    std::vector<glm::vec4> frustumCorners;
    for (uint32_t x = 0; x < 2; ++x)
    {
        for (uint32_t y = 0; y < 2; ++y)
        {
            for (uint32_t z = 0; z < 2; ++z)
            {
                glm::vec4 point = inv * glm::vec4 {
                    2.f * static_cast<float>(x) - 1.f,
                    2.f * static_cast<float>(y) - 1.f,
                    static_cast<float>(z),
                    1.f
                };

                frustumCorners.push_back(point / point.w);
            }
        }
    }

    return frustumCorners;
}

inline glm::mat4 calcMatrices(float fovY, float aspect, float nearPlane, float farPlane,
                              const glm::mat4& camView, glm::vec3 lightDir,
                              float zScalar)
{
    glm::mat4 proj = glm::perspective(glm::radians(fovY),
                                      aspect,
                                      nearPlane,
                                      farPlane);
    auto frustumCorners = getFrustumCornersWS(proj * camView);

    glm::vec3 center {};
    for (const glm::vec4& point : frustumCorners)
        center += glm::vec3(point);
    center /= static_cast<float>(frustumCorners.size());

    lightDir = glm::normalize(lightDir);
    glm::vec3 up = glm::abs(glm::dot(lightDir, glm::vec3(0.f, 1.f, 0.f))) > 0.99f
                   ? glm::vec3(0.f, 0.f, 1.f)
                   : glm::vec3(0.f, 1.f, 0.f);

    glm::mat4 view = glm::lookAt(center - lightDir, center, up);

    float minX = FLT_MAX;
    float maxX = -FLT_MAX;
    float minY = FLT_MAX;
    float maxY = -FLT_MAX;
    float minZ = FLT_MAX;
    float maxZ = -FLT_MAX;

    for (const glm::vec4& point : frustumCorners)
    {
        glm::vec4 pVS = view * point;

        minX = glm::min(minX, pVS.x);
        maxX = glm::max(maxX, pVS.x);
        minY = glm::min(minY, pVS.y);
        maxY = glm::max(maxY, pVS.y);
        minZ = glm::min(minZ, pVS.z);
        maxZ = glm::max(maxZ, pVS.z);
    }

    minZ *= minZ < 0 ? zScalar : 1.f / zScalar;
    maxZ *= maxZ < 0 ? 1.f / zScalar : zScalar;

    glm::mat4 lightProj = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
    lightProj[1][1] *= -1.f;

    return lightProj * view;
}

inline std::vector<glm::vec2> calcCascadePlanes(float nearPlane, float farPlane, int cascadeCount)
{
    constexpr float CascadeSplitLambda = 0.5f;

    std::vector<glm::vec2> cascadePlanes;
    cascadePlanes.resize(cascadeCount);

    for (int i = 0; i < cascadeCount; ++i)
    {
        float p = static_cast<float>(i + 1) / static_cast<float>(cascadeCount);
        float logSplit = nearPlane * glm::pow(farPlane / nearPlane, p);
        float linearSplit = nearPlane + (farPlane - nearPlane) * p;
        float split = CascadeSplitLambda * (logSplit - linearSplit) + linearSplit;

        float cascadeNear = (i == 0) ? nearPlane : cascadePlanes[i - 1][1];
        float cascadeFar = split;

        cascadePlanes.at(i) = glm::vec2(cascadeNear, cascadeFar);
    }

    return cascadePlanes;
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
