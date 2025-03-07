//
// Created by Gianni on 26/01/2025.
//

#include "mesh_node.hpp"

MeshNode::MeshNode()
    : GraphNode()
{
}

MeshNode::MeshNode(NodeType type, const std::string &name,
                   glm::vec3 translation, glm::vec3 rotation, glm::vec3 scale,
                   GraphNode *parent,
                   uuid32_t modelID, std::vector<uuid32_t> meshIDs)
    : GraphNode(type, name, translation, rotation, scale, parent, modelID)
    , mMeshIDs(meshIDs)
{
}

void MeshNode::updateGlobalTransform()
{
    if (mDirty)
    {
        calcLocalTransform();

        if (mParent)
        {
            mGlobalTransform = mParent->globalTransform() * mLocalTransform;
        }
        else
        {
            mGlobalTransform = mLocalTransform;
        }

        mDirty = false;

        for (uint32_t meshID : mMeshIDs)
        {
            Message message = Message::meshInstanceUpdate(meshID, mID, mGlobalTransform);
            SNS::publishMessage(Topic::Type::SceneGraph, message);
        }
    }

    for (auto child : mChildren)
        child->updateGlobalTransform();
}

const std::vector<uuid32_t> &MeshNode::meshIDs() const
{
    return mMeshIDs;
}
