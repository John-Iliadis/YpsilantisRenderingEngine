//
// Created by Gianni on 26/01/2025.
//

#ifndef OPENGLRENDERINGENGINE_SCENE_NODE_HPP
#define OPENGLRENDERINGENGINE_SCENE_NODE_HPP

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

class SceneNode : public SubscriberSNS
{
public:
    SceneNode();
    SceneNode(NodeType type, const std::string& name, const glm::mat4& transformation, SceneNode* parent);
    virtual ~SceneNode();

    void setParent(SceneNode* parent);
    void addChild(SceneNode* child);
    void removeChild(SceneNode* child);
    void orphan();
    void markDirty();

    uuid64_t id() const;
    NodeType type() const;
    const std::string& name() const;
    const std::multiset<SceneNode*>& children() const;

    const glm::mat4& localTransform() const;
    const glm::mat4& globalTransform();
    void setLocalTransform(const glm::mat4& transform);
    virtual void updateGlobalTransform();

    bool operator<(const SceneNode* other) const;

protected:
    uuid64_t mID;
    NodeType mType;
    std::string mName;
    glm::mat4 mLocalTransform;
    glm::mat4 mGlobalTransform;
    bool mDirty;

    SceneNode* mParent;
    std::multiset<SceneNode*> mChildren;
};

#endif //OPENGLRENDERINGENGINE_SCENE_NODE_HPP
