//
// Created by Gianni on 26/01/2025.
//

#ifndef VULKANRENDERINGENGINE_SCENE_NODE_HPP
#define VULKANRENDERINGENGINE_SCENE_NODE_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../app/uuid_registry.hpp"
#include "../app/simple_notification_service.hpp"
#include "../renderer/instanced_mesh.hpp"
#include "../utils/utils.hpp"

enum class NodeType
{
    Empty,
    Mesh,
    DirectionalLight,
    PointLight,
    SpotLight
};

class GraphNode
{
public:
    GraphNode();
    GraphNode(NodeType type,
              const std::string& name,
              const glm::mat4& transformation,
              GraphNode* parent,
              std::optional<uuid32_t> modelID = {});
    virtual ~GraphNode();

    void setParent(GraphNode* parent);
    void addChild(GraphNode* child);
    void removeChild(GraphNode* child);
    void orphan();
    void markDirty();

    uuid32_t id() const;
    NodeType type() const;
    const std::string& name() const;
    std::optional<uuid32_t> modelID() const;
    GraphNode* parent() const;
    const std::vector<GraphNode*>& children() const;

    const glm::mat4& localTransform() const;
    const glm::mat4& globalTransform();
    void setName(const std::string& name);
    void setLocalTransform(const glm::mat4& transform);
    virtual void updateGlobalTransform();

protected:
    uuid32_t mID;
    NodeType mType;
    std::string mName;
    std::optional<uuid32_t> mModelID;
    glm::mat4 mLocalTransform;
    glm::mat4 mGlobalTransform;
    bool mDirty;

    GraphNode* mParent;
    std::vector<GraphNode*> mChildren;
};

#endif //VULKANRENDERINGENGINE_SCENE_NODE_HPP
