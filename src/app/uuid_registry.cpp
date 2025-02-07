//
// Created by Gianni on 2/02/2025.
//

#include "uuid_registry.hpp"

uuid64_t UUIDRegistry::generateID(ObjectType type)
{
    static uuid64_t counter = 10;
    mIdToType.emplace(counter, type);
    return counter++;
}

std::optional<ObjectType> UUIDRegistry::getObjectType(uuid64_t id)
{
    if (mIdToType.contains(id))
        return mIdToType.at(id);
    return std::nullopt;
}

uuid64_t UUIDRegistry::generateModelID()
{
    return generateID(ObjectType::Model);
}

uuid64_t UUIDRegistry::generateTexture2dID()
{
    return generateID(ObjectType::Texture2D);
}

uuid64_t UUIDRegistry::generateTextureCubeID()
{
    return generateID(ObjectType::TextureCube);
}

uuid64_t UUIDRegistry::generateMaterialID()
{
    return generateID(ObjectType::Material);
}

uuid64_t UUIDRegistry::generateMeshID()
{
    return generateID(ObjectType::Mesh);
}

uuid64_t UUIDRegistry::generateSceneNodeID()
{
    return generateID(ObjectType::SceneNode);
}

uuid64_t UUIDRegistry::getDefMatID()
{
    static const uuid64_t defaultMaterialID = generateMaterialID();
    return defaultMaterialID;
}

uuid64_t UUIDRegistry::getDefTexID(MatTexType matTexType)
{
    static const uuid64_t baseColor = generateTexture2dID();
    static const uuid64_t metallicRoughness = generateTexture2dID();
    static const uuid64_t normal = generateTexture2dID();
    static const uuid64_t ao = generateTexture2dID();
    static const uuid64_t emission = generateTexture2dID();

    switch (matTexType)
    {
        case MatTexType::BaseColor: return baseColor;
        case MatTexType::MetallicRoughness: return metallicRoughness;
        case MatTexType::Normal: return normal;
        case MatTexType::Ao: return ao;
        case MatTexType::Emission: return emission;
        default: assert(false);
    }
}

bool UUIDRegistry::is(uuid64_t id, ObjectType objectType)
{
    if (mIdToType.contains(id))
    {
        if (mIdToType.at(id) == objectType)
            return true;
    }

    return false;
}

bool UUIDRegistry::isModel(uuid64_t id)
{
    return is(id, ObjectType::Model);
}

bool UUIDRegistry::isMaterial(uuid64_t id)
{
    return is(id, ObjectType::Material);
}

bool UUIDRegistry::isTexture2D(uuid64_t id)
{
    return is(id, ObjectType::Texture2D);
}

bool UUIDRegistry::isTextureCube(uuid64_t id)
{
    return is(id, ObjectType::TextureCube);
}

bool UUIDRegistry::isMesh(uuid64_t id)
{
    return is(id, ObjectType::Mesh);
}

bool UUIDRegistry::isSceneNode(uuid64_t id)
{
    return is(id, ObjectType::SceneNode);
}
