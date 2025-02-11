//
// Created by Gianni on 26/01/2025.
//

#include "mesh_node.hpp"

MeshNode::MeshNode()
    : GraphNode()
    , mMeshID()
{
}

MeshNode::MeshNode(NodeType type, const std::string& name, const glm::mat4& transformation, GraphNode* parent,
                   uuid32_t modelID, uuid32_t meshID)
    : GraphNode(type, name, transformation, parent, modelID)
    , mMeshID(meshID)
{
}

MeshNode::~MeshNode()
{
    SNS::publishMessage(Topic::Type::SceneGraph, Message::RemoveMeshInstance(mMeshID, mID));
}

void MeshNode::updateGlobalTransform()
{
    if (mDirty)
    {
        if (mParent)
        {
            mGlobalTransform = mParent->globalTransform() * mLocalTransform;
        }
        else
        {
            mGlobalTransform = mLocalTransform;
        }

        mDirty = false;

        SNS::publishMessage(Topic::Type::SceneGraph,
                            Message::meshInstanceUpdate(mMeshID, mID, mGlobalTransform));
    }

    for (auto child : mChildren)
        child->updateGlobalTransform();
}

uuid32_t MeshNode::meshID() const
{
    return mMeshID;
}
