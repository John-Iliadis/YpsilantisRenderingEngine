//
// Created by Gianni on 27/01/2025.
//

#include "scene_graph.hpp"

SceneGraph::SceneGraph()
    : SubscriberSNS({Topic::Type::Resources})
    , mRoot(NodeType::Empty, "RootNode", glm::identity<glm::mat4>(), nullptr)
{
}

SceneNode *SceneGraph::searchNode(uuid64_t nodeID)
{
    std::vector<SceneNode*> stack(1, &mRoot);

    while (!stack.empty())
    {
        SceneNode* node = stack.back();
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
        std::vector<SceneNode*> stack(1, &mRoot);

        while (!stack.empty())
        {
            SceneNode* node = stack.back();
            stack.pop_back();

            if (node->type() == NodeType::Mesh)
            {
                MeshNode* meshNode = dynamic_cast<MeshNode*>(node);

                if (m->meshIDs.contains(meshNode->meshID()))
                {
                    meshNode->orphan();
                    delete meshNode;
                    continue;
                }
            }

            for (auto child : node->children())
                stack.push_back(child);
        }
    }
}

void SceneGraph::addNode(SceneNode *node)
{
    mRoot.addChild(node);
}

void SceneGraph::deleteNode(uuid64_t nodeID)
{
    SceneNode* node = searchNode(nodeID);
    node->orphan();
    delete node;
}
