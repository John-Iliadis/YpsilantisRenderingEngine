//
// Created by Gianni on 2/02/2025.
//

#include "uuid_registry.hpp"

uuid32_t UUIDRegistry::generateID(ObjectType type)
{
    static uuid32_t counter = 10;
    mIdToType.emplace(counter, type);
    return counter++;
}

std::optional<ObjectType> UUIDRegistry::getObjectType(uuid32_t id)
{
    if (mIdToType.contains(id))
        return mIdToType.at(id);
    return std::nullopt;
}

uuid32_t UUIDRegistry::generateModelID()
{
    return generateID(ObjectType::Model);
}

uuid32_t UUIDRegistry::generateMeshID()
{
    return generateID(ObjectType::Mesh);
}

uuid32_t UUIDRegistry::generateSceneNodeID()
{
    return generateID(ObjectType::SceneNode);
}

bool UUIDRegistry::is(uuid32_t id, ObjectType objectType)
{
    if (mIdToType.contains(id))
    {
        if (mIdToType.at(id) == objectType)
            return true;
    }

    return false;
}

bool UUIDRegistry::isModel(uuid32_t id)
{
    return is(id, ObjectType::Model);
}

bool UUIDRegistry::isMesh(uuid32_t id)
{
    return is(id, ObjectType::Mesh);
}

bool UUIDRegistry::isSceneNode(uuid32_t id)
{
    return is(id, ObjectType::SceneNode);
}
