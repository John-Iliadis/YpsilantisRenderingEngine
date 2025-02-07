//
// Created by Gianni on 27/01/2025.
//

#ifndef OPENGLRENDERINGENGINE_SCENE_GRAPH_HPP
#define OPENGLRENDERINGENGINE_SCENE_GRAPH_HPP

#include "../app/simple_notification_service.hpp"
#include "mesh_node.hpp"

class SceneGraph : public SubscriberSNS
{
public:
    SceneGraph();

    void addNode(SceneNode* node);
    void deleteNode(uuid64_t nodeID);
    void updateTransforms();

    void notify(const Message &message) override;

    SceneNode* searchNode(uuid64_t nodeID);

public:
    SceneNode mRoot;
};

#endif //OPENGLRENDERINGENGINE_SCENE_GRAPH_HPP
