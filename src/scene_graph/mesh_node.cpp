//
// Created by Gianni on 26/01/2025.
//

#include "mesh_node.hpp"

MeshNode::MeshNode()
    : SceneNode()
    , mMeshID()
    , mMatIndex()
    , mInstanceID()
    , mModifiedMaterial()
{
    subscribe(Topic::Type::Resources);
}

MeshNode::MeshNode(NodeType type, const std::string& name, const glm::mat4& transformation, SceneNode* parent,
                   uuid64_t meshID, uint32_t instanceID, index_t materialIndex, const std::string& matName)
    : SceneNode(type, name, transformation, parent)
    , mMeshID(meshID)
    , mInstanceID(instanceID)
    , mMatIndex(materialIndex)
    , mMatName(matName)
    , mModifiedMaterial()
{
    subscribe(Topic::Type::Resources);
}

MeshNode::~MeshNode()
{
    SNS::publishMessage(Topic::Type::SceneGraph, Message::create<Message::RemoveMeshInstance>(mMeshID, mInstanceID));
}

void MeshNode::notify(const Message &message)
{
    if (const auto m = message.getIf<Message::MaterialDeleted>())
    {
        if (m->removeIndex == mMatIndex)
            mMatIndex = 0;

        if (m->transferIndex.has_value() && m->transferIndex == mMatIndex)
            mMatIndex = m->removeIndex;
    }

    if (const auto m = message.getIf<Message::MaterialRemap>())
    {
        if (!mModifiedMaterial && mMatName == m->matName)
        {
            mMatIndex = m->newMatIndex;
            SNS::publishMessage(Topic::Type::SceneGraph, Message::create<Message::MeshInstanceUpdate>(mMeshID, mID, mInstanceID, mMatIndex, mGlobalTransform));
        }
    }
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

        SNS::publishMessage(Topic::Type::SceneGraph, Message::create<Message::MeshInstanceUpdate>(mMeshID, mID, mInstanceID, mMatIndex, mGlobalTransform));
    }

    for (auto child : mChildren)
        child->updateGlobalTransform();
}

uuid64_t MeshNode::meshID() const
{
    return mMeshID;
}

uint32_t MeshNode::instanceID() const
{
    return mInstanceID;
}
