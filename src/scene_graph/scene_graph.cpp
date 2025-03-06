//
// Created by Gianni on 27/01/2025.
//

#include "scene_graph.hpp"

SceneGraph::SceneGraph()
    : SubscriberSNS({Topic::Type::Renderer})
    , mRoot(NodeType::Empty, "RootNode", {}, {}, glm::vec3(1.f), nullptr)
{
}

GraphNode *SceneGraph::searchNode(uuid32_t nodeID)
{
    std::vector<GraphNode*> stack(1, &mRoot);

    while (!stack.empty())
    {
        GraphNode* node = stack.back();
        stack.pop_back();

        if (node->id() == nodeID)
            return node;

        for (auto child : node->children())
            stack.push_back(child);
    }

    return nullptr;
}

void SceneGraph::updateTransforms()
{
    mRoot.updateGlobalTransform();
}

void SceneGraph::notify(const Message &message)
{
    if (const auto& m = message.getIf<Message::ModelDeleted>())
    {
        std::vector<GraphNode*> stack(1, &mRoot);

        while (!stack.empty())
        {
            GraphNode* node = stack.back();
            stack.pop_back();

            if (node->modelID().has_value() && node->modelID() == m->modelID)
            {
                node->orphan();
                delete node;
                continue;
            }

            for (auto child : node->children())
                stack.push_back(child);
        }
    }
}

void SceneGraph::addNode(GraphNode *node)
{
    mRoot.addChild(node);
}

void SceneGraph::deleteNode(uuid32_t nodeID)
{
    GraphNode* node = searchNode(nodeID);
    node->orphan();
    delete node;
}

bool SceneGraph::hasDescendant(GraphNode *current, GraphNode *descendant)
{
    for (auto child : current->children())
    {
        if (child == descendant || hasDescendant(child, descendant))
            return true;
    }

    return false;
}

GraphNode *SceneGraph::root()
{
    return &mRoot;
}

std::vector<GraphNode *> &SceneGraph::topLevelNodes()
{
    return const_cast<std::vector<GraphNode*>&>(mRoot.children());
}
