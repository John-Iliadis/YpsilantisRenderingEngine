//
// Created by Gianni on 26/01/2025.
//

#include "mesh_node.hpp"

MeshNode::MeshNode()
    : GraphNode()
    , mMeshID()
    , mInstanceID()
{
}

MeshNode::MeshNode(NodeType type, const std::string& name, const glm::mat4& transformation, GraphNode* parent,
                   uuid32_t meshID, uint32_t instanceID)
    : GraphNode(type, name, transformation, parent)
    , mMeshID(meshID)
    , mInstanceID(instanceID)
{
}

MeshNode::~MeshNode()
{
    SNS::publishMessage(Topic::Type::SceneGraph, Message::create<Message::RemoveMeshInstance>(mMeshID, mInstanceID));
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

        SNS::publishMessage(Topic::Type::SceneGraph, Message::create<Message::MeshInstanceUpdate>(mMeshID, mID, mInstanceID, mGlobalTransform));
    }

    for (auto child : mChildren)
        child->updateGlobalTransform();
}

uuid32_t MeshNode::meshID() const
{
    return mMeshID;
}

uint32_t MeshNode::instanceID() const
{
    return mInstanceID;
}
