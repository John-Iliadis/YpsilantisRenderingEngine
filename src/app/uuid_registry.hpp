//
// Created by Gianni on 2/02/2025.
//

#ifndef VULKANRENDERINGENGINE_UUID_REGISTRY_HPP
#define VULKANRENDERINGENGINE_UUID_REGISTRY_HPP

#include "types.hpp"
#include "../renderer/material.hpp"

enum class ObjectType
{
    Model,
    Material,
    Texture,
    Mesh,
    SceneNode
};

class UUIDRegistry
{
public:
    static uuid64_t generateModelID();
    static uuid64_t generateTextureID();
    static uuid64_t generateMaterialID();
    static uuid64_t generateMeshID();
    static uuid64_t generateSceneNodeID();
    static std::optional<ObjectType> getObjectType(uuid64_t id);

    static uuid64_t getDefMatID();
    static uuid64_t getDefTexID(MatTexType matTexType);

private:
    static uuid64_t generateID(ObjectType type);
    static inline std::unordered_map<uuid64_t, ObjectType> mIdToType;
};

#endif //VULKANRENDERINGENGINE_UUID_REGISTRY_HPP
