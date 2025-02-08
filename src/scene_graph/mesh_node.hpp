//
// Created by Gianni on 26/01/2025.
//

#ifndef OPENGLRENDERINGENGINE_MESH_NODE_HPP
#define OPENGLRENDERINGENGINE_MESH_NODE_HPP

#include "graph_node.hpp"

class MeshNode : public GraphNode
{
public:
    MeshNode();
    MeshNode(NodeType type, const std::string& name, const glm::mat4& transformation, GraphNode* parent,
             uuid32_t meshID, uint32_t instanceID);
    ~MeshNode();

    void updateGlobalTransform() override;

    uuid32_t meshID() const;
    uint32_t instanceID() const;

private:
    uuid32_t mMeshID;
    uint32_t mInstanceID;
};

#endif //OPENGLRENDERINGENGINE_MESH_NODE_HPP
