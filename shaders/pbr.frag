#version 460 core

#include "material.glsl"
#include "lights.glsl"
#include "brdf.glsl"

layout (early_fragment_tests) in;

layout (location = 0) in vec3 vFragWorldPos;
layout (location = 1) in vec2 vTexCoords;
layout (location = 2) in mat3 vTBN; // todo potential issues with this mat

layout (location = 0) out vec4 outFragColor;

layout (push_constant) uniform PushConstants
{
    uint dirLightCount;
    uint pointLightCount;
    uint spotLightCount;
};

layout (set = 0, binding = 0) uniform CameraUBO
{
    mat4 view;
    mat4 projection;
    mat4 viewProj;
    vec4 cameraPos;
    vec4 cameraDir;
    float nearPlane;
    float farPlane;
};

layout (set = 1, binding = 0) uniform sampler2D ssaoTexture;

layout (set = 2, binding = 0) buffer readonly DirLightsSSBO { DirectionalLight dirLights[]; };
layout (set = 2, binding = 1) buffer readonly PointLightsSSBO { PointLight pointLights[]; };
layout (set = 2, binding = 2) buffer readonly SpotLightSSBO { SpotLight spotLights[]; };

layout (set = 3, binding = 0) uniform MaterialsUBO { Material material; };
layout (set = 3, binding = 1) uniform sampler2D baseColorTex;
layout (set = 3, binding = 2) uniform sampler2D metallicTex;
layout (set = 3, binding = 3) uniform sampler2D roughnessTex;
layout (set = 3, binding = 4) uniform sampler2D normalTex;
layout (set = 3, binding = 5) uniform sampler2D aoTex;
layout (set = 3, binding = 6) uniform sampler2D emissionTex;

// todo: update light dir when using editor
// todo: add light intensity
// todo: put brdf calculation in a function
// PBR requires all inputs to be linear
void main()
{
    vec2 texCoords = vTexCoords * material.tiling + material.offset;

    vec3 baseColor = texture(baseColorTex, texCoords).rgb * material.baseColorFactor.rgb;
    float metallic = texture(metallicTex, texCoords).b * material.metallicFactor;
    float roughness = texture(roughnessTex, texCoords).g * material.roughnessFactor;
    vec3 normalSample = normalize(texture(normalTex, texCoords).xyz * 2.0 - 1.0);
    float ao = texture(aoTex, texCoords).r;
    vec3 emission = texture(emissionTex, texCoords).rgb * material.emissionFactor.rgb;

    vec3 normal = normalize(vTBN * normalSample);
    vec3 viewVec = normalize(cameraPos.xyz - vFragWorldPos);

    vec3 L_0 = vec3(0.0);
    vec3 F_0 = mix(vec3(0.04), baseColor, metallic);

    for (uint i = 0; i < dirLightCount; ++i)
    {
        vec3 lightVec = -normalize(dirLights[i].direction.xyz);
        vec3 halfwayVec = normalize(viewVec + lightVec);
        vec3 lightRadiance = dirLights[i].color.rgb * dirLights[i].intensity;

        float distribution = distributionGGX(normal, halfwayVec, roughness);
        float geometry = geometrySmith(normal, viewVec, lightVec, roughness);
        vec3 frensel = fresnelSchlick(viewVec, halfwayVec, F_0);

        vec3 BRDF_Numerator = distribution * geometry * frensel;
        float BRDF_Denominator = 4 * max(dot(viewVec, normal), 0.0) * max(dot(lightVec, normal), 0.0) + 1e-5;
        vec3 BRDF = BRDF_Numerator / BRDF_Denominator;

        vec3 diffuseTerm = baseColor * (vec3(1.0) - frensel) * (1.0 - metallic) / PI;

        L_0 += (diffuseTerm + BRDF) * lightRadiance * max(dot(normal, lightVec), 0.0);
    }

    vec3 ambient = vec3(0.03) * baseColor * ao;

    outFragColor = vec4(L_0 + ambient, 1.0);
}
