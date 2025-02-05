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

    void updateTransforms();
    void notify(const Message &message) override;

public:
    SceneNode mRoot;
};

#endif //OPENGLRENDERINGENGINE_SCENE_GRAPH_HPP
