//
// Created by Gianni on 26/01/2025.
//

#include "graph_node.hpp"

GraphNode::GraphNode()
    : mID(UUIDRegistry::generateSceneNodeID())
    , mType(NodeType::Empty)
    , mName("")
    , mTranslation()
    , mRotation()
    , mScale(1.f)
    , mDirty()
    , mParent()
{
}

GraphNode::GraphNode(NodeType type, const std::string& name,
                     glm::vec3 translation, glm::vec3 rotation, glm::vec3 scale,
                     GraphNode* parent,
                     std::optional<uuid32_t> modelID)
    : mID(UUIDRegistry::generateSceneNodeID())
    , mType(type)
    , mName(name)
    , mModelID(modelID)
    , mTranslation(translation)
    , mRotation(rotation)
    , mScale(scale)
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
    mChildren.push_back(child);
}

void GraphNode::removeChild(GraphNode *child)
{
    for (uint32_t i = 0; i < mChildren.size(); ++i)
        if (mChildren.at(i) == child)
            mChildren.erase(mChildren.begin() + i);
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

GraphNode *GraphNode::parent() const
{
    return mParent;
}

const std::vector<GraphNode*>& GraphNode::children() const
{
    return mChildren;
}

const glm::mat4 &GraphNode::localTransform() const
{
    return mLocalTransform;
}

const glm::mat4 &GraphNode::globalTransform() const
{
    return mGlobalTransform;
}

void GraphNode::setLocalTransform(const glm::mat4 &transform)
{
    mLocalTransform = transform;
    markDirty();
}

void GraphNode::updateGlobalTransform()
{
    if (mDirty)
    {
        calcLocalTransform();

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

float *GraphNode::translationPtr()
{
    return glm::value_ptr(mTranslation);
}

float *GraphNode::rotationPtr()
{
    return glm::value_ptr(mRotation);
}

float *GraphNode::scalePtr()
{
    return glm::value_ptr(mScale);
}

void GraphNode::calcLocalTransform()
{
    glm::quat quat = glm::quat(glm::radians(mRotation));
    glm::mat4 rot = glm::toMat4(quat);

    mLocalTransform = glm::translate(glm::identity<glm::mat4>(), mTranslation) * rot;
    mLocalTransform = glm::scale(mLocalTransform, mScale);
}
