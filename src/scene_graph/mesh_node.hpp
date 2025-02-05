//
// Created by Gianni on 26/01/2025.
//

#ifndef OPENGLRENDERINGENGINE_MESH_NODE_HPP
#define OPENGLRENDERINGENGINE_MESH_NODE_HPP

#include "scene_node.hpp"

class MeshNode : public SceneNode
{
public:
    MeshNode();
    MeshNode(NodeType type, const std::string& name, const glm::mat4& transformation, SceneNode* parent,
             uuid64_t meshID, uint32_t instanceID, index_t materialIndex, const std::string& matName);
    ~MeshNode();

    void notify(const Message &message) override;
    void updateGlobalTransform() override;

    uuid64_t meshID() const;
    uint32_t instanceID() const;

private:
    uuid64_t mMeshID;
    uint32_t mInstanceID;
    index_t mMatIndex;
    std::string mMatName;
    bool mModifiedMaterial;
};

#endif //OPENGLRENDERINGENGINE_MESH_NODE_HPP
