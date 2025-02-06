//
// Created by Gianni on 19/01/2025.
//

#include "model.hpp"

Model::Model()
    : SubscriberSNS({Topic::Type::SceneGraph})
    , root()
    , bb()
{
}

void Model::notify(const Message &message)
{
    if (const auto m = message.getIf<Message::MaterialDeleted>())
    {
        for (auto& [materialName, materialID] : mappedMaterials)
        {
            if (materialID == m->materialID)
                materialID = UUIDRegistry::getDefMatID();
        }
    }
}

std::optional<uuid64_t> Model::getMaterialID(const Model::Node &node) const
{
    if (node.materialName.has_value())
        return mappedMaterials.at(node.materialName.value());
    return std::nullopt;
}

void Model::remapMaterial(const std::string &materialName, uuid64_t newMatID, uint32_t newMatIndex)
{
    if (mappedMaterials.contains(materialName))
    {
        mappedMaterials.at(materialName) = newMatID;

        SNS::publishMessage(Topic::Type::Resources, Message::create<Message::MaterialRemap>(newMatIndex, materialName));
    }
}

static void getModelMeshesRecursive(const Model::Node& node, std::unordered_set<uuid64_t>& meshIDs)
{
    if (auto meshID = node.meshID)
        meshIDs.insert(*meshID);

    for (const auto& child : node.children)
        getModelMeshesRecursive(child, meshIDs);
}

std::unordered_set<uuid64_t> getModelMeshIDs(const Model& model)
{
    std::unordered_set<uuid64_t> modelMeshes;

    getModelMeshesRecursive(model.root, modelMeshes);

    return modelMeshes;
}
