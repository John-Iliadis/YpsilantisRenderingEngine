//
// Created by Gianni on 13/01/2025.
//

#include "scene_node.hpp"

SceneNode::SceneNode()
    : SceneNode(nullptr, nullptr)
{
}

SceneNode::SceneNode(SceneNode *parent, SceneObject *sceneObject)
    : mParent(parent)
    , mSceneObject(sceneObject)
{
}

SceneNode::~SceneNode()
{
    delete mSceneObject;
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

const std::string &SceneNode::name() const
{
    return mSceneObject->name;
}

const std::multiset<SceneNode*>& SceneNode::children() const
{
    return mChildren;
}

bool SceneNode::operator<(const SceneNode* other) const
{
    return std::less<std::string>()(mSceneObject->name, other->mSceneObject->name);
}
