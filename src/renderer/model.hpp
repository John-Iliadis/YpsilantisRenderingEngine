//
// Created by Gianni on 13/01/2025.
//

#ifndef VULKANRENDERINGENGINE_MODEL_HPP
#define VULKANRENDERINGENGINE_MODEL_HPP

#include <glm/glm.hpp>
#include "../app/simple_notification_service.hpp"
#include "../app/uuid_registry.hpp"
#include "bounding_box.hpp"

class Model : public SubscriberSNS
{
public:
    struct Node
    {
        std::string name;
        glm::mat4 transformation;
        std::optional<uuid64_t> meshID;
        std::optional<std::string> materialName;
        std::vector<Node> children;
    };

public:
    Node root;
    BoundingBox bb;
    std::unordered_map<std::string, uuid64_t> mappedMaterials;

public:
    Model();

    void notify(const Message &message) override;
    void remapMaterial(const std::string& materialName, uuid64_t newMatID, uint32_t newMatIndex);

    std::optional<uuid64_t> getMaterialID(const Model::Node& node) const;
};

std::unordered_set<uuid64_t> getModelMeshIDs(const Model& model);

#endif //VULKANRENDERINGENGINE_MODEL_HPP
