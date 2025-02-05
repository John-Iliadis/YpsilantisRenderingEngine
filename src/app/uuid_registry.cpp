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

uuid64_t UUIDRegistry::generateTextureID()
{
    return generateID(ObjectType::Texture);
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
    static const uuid64_t baseColor = generateTextureID();
    static const uuid64_t metallicRoughness = generateTextureID();
    static const uuid64_t normal = generateTextureID();
    static const uuid64_t ao = generateTextureID();
    static const uuid64_t emission = generateTextureID();

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
