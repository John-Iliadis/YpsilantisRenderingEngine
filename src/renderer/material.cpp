//
// Created by Gianni on 22/02/2025.
//

#include "material.hpp"

bool hasTextures(const Material& material)
{
    return material.baseColorTexIndex != -1 ||
           material.metallicTexIndex != -1 ||
           material.roughnessTexIndex != -1 ||
           material.normalTexIndex != -1 ||
           material.aoTexIndex != -1 ||
           material.emissionTexIndex != -1;
}

void createDefaultMaterialTextures(const VulkanRenderDevice& renderDevice)
{
    uint32_t baseColorTexData = 0xffffffff;
    uint32_t metallicTexData = 0x00000000;
    uint32_t roughnessTexData = 0xffffffff;
    uint32_t normalTexData = 0x8080ff00;
    uint32_t aoTexData = 0xffffffff;
    uint32_t emissionTexData = 0x00000000;

    TextureSpecification specification {
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .width = 1,
        .height = 1,
        .layerCount = 1,
        .imageViewType = VK_IMAGE_VIEW_TYPE_2D,
        .imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                      VK_IMAGE_USAGE_SAMPLED_BIT,
        .imageAspect = VK_IMAGE_ASPECT_COLOR_BIT,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .magFilter = TextureMagFilter::Nearest,
        .minFilter = TextureMinFilter::Nearest,
        .wrapS = TextureWrap::Repeat,
        .wrapT = TextureWrap::Repeat,
        .wrapR = TextureWrap::Repeat,
        .generateMipMaps = false
    };

    defaultBaseColorTex = {renderDevice, specification, &baseColorTexData};
    defaultMetallicTex = {renderDevice, specification, &metallicTexData};
    defaultRoughnessTex = {renderDevice, specification, &roughnessTexData};
    defaultNormalTex = {renderDevice, specification, &normalTexData};
    defaultAoTex = {renderDevice, specification, &aoTexData};
    defaultEmissionTex = {renderDevice, specification, &emissionTexData};
}

void destroyDefaultMaterialTextures()
{
    defaultBaseColorTex = {};
    defaultMetallicTex = {};
    defaultRoughnessTex = {};
    defaultNormalTex = {};
    defaultAoTex = {};
    defaultEmissionTex = {};
}
