//
// Created by Gianni on 2/02/2025.
//

#ifndef VULKANRENDERINGENGINE_UUID_REGISTRY_HPP
#define VULKANRENDERINGENGINE_UUID_REGISTRY_HPP

#include "types.hpp"

enum class ObjectType
{
    Model,
    Mesh,
    SceneNode
};

class UUIDRegistry
{
public:
    static uuid32_t generateModelID();
    static uuid32_t generateMeshID();
    static uuid32_t generateSceneNodeID();
    static std::optional<ObjectType> getObjectType(uuid32_t id);

    static bool is(uuid32_t id, ObjectType objectType);
    static bool isModel(uuid32_t id);
    static bool isMesh(uuid32_t id);
    static bool isSceneNode(uuid32_t id);

private:
    static uuid32_t generateID(ObjectType type);
    static inline std::unordered_map<uuid32_t, ObjectType> mIdToType;
};

#endif //VULKANRENDERINGENGINE_UUID_REGISTRY_HPP
