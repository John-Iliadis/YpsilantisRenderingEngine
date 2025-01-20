//
// Created by Gianni on 19/01/2025.
//

#ifndef VULKANRENDERINGENGINE_LOADED_RESOURCE_HPP
#define VULKANRENDERINGENGINE_LOADED_RESOURCE_HPP

#include <assimp/scene.h>
#include "../material.hpp"
#include "../model.hpp"

void stbi_image_free(void*);

using TexturePath = std::string;

template <typename PixelFormat>
struct LoadedTexture
{
    std::string name;
    uint32_t width;
    uint32_t height;
    std::shared_ptr<PixelFormat> pixels;

    LoadedTexture()
        : LoadedTexture("", TextureType::None, 0, 0, nullptr)
    {
    }

    LoadedTexture(const std::string& name, uint32_t width, uint32_t height, PixelFormat* pixels)
        : name(name)
        , width(width)
        , height(height)
        , pixels(pixels, [] (PixelFormat* pixels) {stbi_image_free(reinterpret_cast<void*>(pixels));})
    {
    }
};

struct LoadedMesh
{
    std::string name;
    std::vector<InstancedMesh::Vertex> vertices;
    std::vector<uint32_t> indices;
    size_t materialIndex;
};

struct LoadedModel
{
    struct Mesh
    {
        std::shared_ptr<InstancedMesh> mesh;
        size_t materialIndex;
    };

    struct Material
    {
        std::string name;
        std::array<TexturePath, TextureType::Count> textures;
        glm::vec3 albedoColor = glm::vec3(1.f);
        glm::vec3 emissionColor = glm::vec3(0.f);
    };

    std::string name;
    ModelNode root;
    std::vector<LoadedModel::Mesh> meshes;
    std::vector<LoadedModel::Material> materials;

    // todo fix: some textures may be missing due to failed load
    std::unordered_map<TexturePath, NamedTexture> textures;
};

#endif //VULKANRENDERINGENGINE_LOADED_RESOURCE_HPP
