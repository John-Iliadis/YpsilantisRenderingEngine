//
// Created by Gianni on 16/01/2025.
//

#ifndef VULKANRENDERINGENGINE_IMPORTER_SCENE_HPP
#define VULKANRENDERINGENGINE_IMPORTER_SCENE_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../scene.hpp"

namespace Importer
{
    struct SceneNode
    {
        glm::mat4 localTransform = glm::identity<glm::mat4>();
        std::vector<std::string> meshes;
        std::vector<SceneNode> children;
    };

    struct Material
    {
        std::string name;
        std::unordered_map<aiTextureType, std::string> mapNames;
//        std::string albedoMapName;
//        std::string roughnessMapName;
//        std::string metallicMapName;
//        std::string normalMapName;
//        std::string heightMapName;
//        std::string aoMapName;
//        std::string emissionMapName;
//        std::string specularMapName;
        glm::vec4 albedoColor;
        glm::vec4 emissionColor;
    };

    struct Mesh
    {
        ::Mesh mesh;
        std::string materialName;
    };

    struct Scene
    {
        std::string name;
        std::string directory;
        SceneNode rootNode;
        std::vector<Importer::Mesh> meshes;
        std::unordered_map<std::string, Material> materials;
        std::unordered_map<std::string, Texture> textures;
    };
}

#endif //VULKANRENDERINGENGINE_IMPORTER_SCENE_HPP
