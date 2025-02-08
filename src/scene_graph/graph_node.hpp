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

class GraphNode : public SubscriberSNS
{
public:
    GraphNode();
    GraphNode(NodeType type, const std::string& name, const glm::mat4& transformation, GraphNode* parent);
    virtual ~GraphNode();

    void setParent(GraphNode* parent);
    void addChild(GraphNode* child);
    void removeChild(GraphNode* child);
    void orphan();
    void markDirty();

    uuid64_t id() const;
    NodeType type() const;
    const std::string& name() const;
    const std::multiset<GraphNode*>& children() const;

    const glm::mat4& localTransform() const;
    const glm::mat4& globalTransform();
    void setLocalTransform(const glm::mat4& transform);
    virtual void updateGlobalTransform();

    bool operator<(const GraphNode* other) const;

protected:
    uuid64_t mID;
    NodeType mType;
    std::string mName;
    glm::mat4 mLocalTransform;
    glm::mat4 mGlobalTransform;
    bool mDirty;

    GraphNode* mParent;
    std::multiset<GraphNode*> mChildren;
};

#endif //VULKANRENDERINGENGINE_SCENE_NODE_HPP
