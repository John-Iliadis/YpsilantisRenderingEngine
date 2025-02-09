//
// Created by Gianni on 26/01/2025.
//

#include "graph_node.hpp"

GraphNode::GraphNode()
    : mID(UUIDRegistry::generateSceneNodeID())
    , mType(NodeType::Empty)
    , mName("")
    , mLocalTransform(glm::identity<glm::mat4>())
    , mGlobalTransform(glm::identity<glm::mat4>())
    , mDirty()
    , mParent()
{
}

GraphNode::GraphNode(NodeType type,
                     const std::string& name,
                     const glm::mat4& transformation,
                     GraphNode* parent,
                     std::optional<uuid32_t> modelID)
    : mID(UUIDRegistry::generateSceneNodeID())
    , mType(type)
    , mName(name)
    , mModelID(modelID)
    , mLocalTransform(transformation)
    , mGlobalTransform(transformation)
    , mDirty(true)
    , mParent(parent)
{
}

GraphNode::~GraphNode()
{
    for (GraphNode* node : mChildren)
        delete node;
}

void GraphNode::setParent(GraphNode *parent)
{
    mParent = parent;
}

void GraphNode::addChild(GraphNode* child)
{
    child->mParent = this;
    mChildren.insert(child);
}

void GraphNode::removeChild(GraphNode *child)
{
    mChildren.erase(child);
}

void GraphNode::orphan()
{
    if (mParent)
    {
        mParent->removeChild(this);
        mParent = nullptr;
    }
}

void GraphNode::markDirty()
{
    mDirty = true;
    for (auto child : mChildren)
        child->markDirty();
}

uuid32_t GraphNode::id() const
{
    return mID;
}

NodeType GraphNode::type() const
{
    return mType;
}

const std::string &GraphNode::name() const
{
    return mName;
}

std::optional<uuid32_t> GraphNode::modelID() const
{
    return mModelID;
}

const std::multiset<GraphNode*>& GraphNode::children() const
{
    return mChildren;
}

const glm::mat4 &GraphNode::localTransform() const
{
    return mLocalTransform;
}

const glm::mat4 &GraphNode::globalTransform()
{
    return mGlobalTransform;
}

void GraphNode::setLocalTransform(const glm::mat4 &transform)
{
    mLocalTransform = transform;
    markDirty();
}

bool GraphNode::operator<(const GraphNode* other) const
{
    return std::less<std::string>()(mName, other->mName);
}

void GraphNode::updateGlobalTransform()
{
    if (mDirty)
    {
        if (mParent)
        {
            mGlobalTransform = mParent->mGlobalTransform * mLocalTransform;
        }
        else
        {
            mGlobalTransform = mLocalTransform;
        }

        mDirty = false;
    }

    for (auto child : mChildren)
        child->updateGlobalTransform();
}

void GraphNode::setName(const std::string &name)
{
    mName = name;
}
