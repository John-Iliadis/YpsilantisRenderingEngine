//
// Created by Gianni on 26/01/2025.
//

#include "scene_node.hpp"

SceneNode::SceneNode()
    : mID(UUIDRegistry::generateSceneNodeID())
    , mType(NodeType::Empty)
    , mName("")
    , mLocalTransform(glm::identity<glm::mat4>())
    , mGlobalTransform(glm::identity<glm::mat4>())
    , mDirty()
    , mParent()
{
}

SceneNode::SceneNode(NodeType type, const std::string &name, const glm::mat4& transformation, SceneNode *parent)
    : mID(UUIDRegistry::generateSceneNodeID())
    , mType(type)
    , mName(name)
    , mLocalTransform(transformation)
    , mGlobalTransform(transformation)
    , mDirty(true)
    , mParent(parent)
{
}

SceneNode::~SceneNode()
{
    for (SceneNode* node : mChildren)
        delete node;
}

void SceneNode::setParent(SceneNode *parent)
{
    mParent = parent;
}

void SceneNode::addChild(SceneNode* child)
{
    child->mParent = this;
    mChildren.insert(child);
}

void SceneNode::removeChild(SceneNode *child)
{
    mChildren.erase(child);
}

void SceneNode::orphan()
{
    mParent->removeChild(this);
    mParent = nullptr;
}

void SceneNode::markDirty()
{
    mDirty = true;
    for (auto child : mChildren)
        child->markDirty();
}

uuid64_t SceneNode::id() const
{
    return mID;
}

NodeType SceneNode::type() const
{
    return mType;
}

const std::string &SceneNode::name() const
{
    return mName;
}

const std::multiset<SceneNode*>& SceneNode::children() const
{
    return mChildren;
}

const glm::mat4 &SceneNode::localTransform() const
{
    return mLocalTransform;
}

const glm::mat4 &SceneNode::globalTransform()
{
    return mGlobalTransform;
}

void SceneNode::setLocalTransform(const glm::mat4 &transform)
{
    mLocalTransform = transform;
    markDirty();
}

bool SceneNode::operator<(const SceneNode* other) const
{
    return std::less<std::string>()(mName, other->mName);
}

void SceneNode::updateGlobalTransform()
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
