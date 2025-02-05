//
// Created by Gianni on 27/01/2025.
//

#include "scene_graph.hpp"

SceneGraph::SceneGraph()
    : mRoot(NodeType::Empty, "RootNode", glm::identity<glm::mat4>(), nullptr)
{
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
                }
            }

            for (auto child : node->children())
                stack.push_back(child);
        }
    }
}
