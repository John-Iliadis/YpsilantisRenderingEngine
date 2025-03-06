//
// Created by Gianni on 26/01/2025.
//

#ifndef VULKANRENDERINGENGINE_SCENE_NODE_HPP
#define VULKANRENDERINGENGINE_SCENE_NODE_HPP

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>
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
    glm::vec3 localT;
    glm::vec3 localR;
    glm::vec3 localS;
    glm::vec3 globalT;
    glm::vec3 globalR;
    glm::vec3 globalS;

public:
    GraphNode();
    GraphNode(NodeType type, const std::string& name,
              glm::vec3 translation, glm::vec3 rotation, glm::vec3 scale,
              GraphNode* parent,
              std::optional<uuid32_t> modelID = {});
    virtual ~GraphNode();

    void setParent(GraphNode* parent);
    void addChild(GraphNode* child);
    void removeChild(GraphNode* child);
    void orphan();
    void markDirty();

    void setName(const std::string& name);
    void calcLocalTransform();
    virtual void updateGlobalTransform();

    uuid32_t id() const;
    NodeType type() const;
    const std::string& name() const;
    std::optional<uuid32_t> modelID() const;
    GraphNode* parent() const;
    const glm::mat4& globalTransform() const;
    const std::vector<GraphNode*>& children() const;

protected:
    uuid32_t mID;
    NodeType mType;
    std::string mName;
    std::optional<uuid32_t> mModelID;
    bool mDirty;

    glm::mat4 mLocalTransform;
    glm::mat4 mGlobalTransform;

    GraphNode* mParent;
    std::vector<GraphNode*> mChildren;
};

#endif //VULKANRENDERINGENGINE_SCENE_NODE_HPP
