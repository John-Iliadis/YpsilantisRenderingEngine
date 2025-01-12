//
// Created by Gianni on 13/01/2025.
//

#ifndef VULKANRENDERINGENGINE_SCENE_NODE_HPP
#define VULKANRENDERINGENGINE_SCENE_NODE_HPP

#include "scene.hpp"

class SceneNode
{
public:
    SceneNode();
    SceneNode(SceneNode* parent, SceneObject* sceneObject);
    ~SceneNode();

    void setParent(SceneNode* parent);
    void addChild(SceneNode* child);
    void removeChild(SceneNode* child);
    void orphan();

    const std::string& name() const;
    const std::multiset<SceneNode*>& children() const;

    bool operator<(const SceneNode* other) const;

private:
    SceneNode* mParent;
    std::multiset<SceneNode*> mChildren;
    SceneObject* mSceneObject;
};

#endif //VULKANRENDERINGENGINE_SCENE_NODE_HPP
