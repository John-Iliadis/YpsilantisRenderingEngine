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
             uuid32_t modelID, std::vector<uuid32_t> meshIDs);
    ~MeshNode();

    void updateGlobalTransform() override;

    const std::vector<uuid32_t>& meshIDs() const;

private:
    std::vector<uuid32_t> mMeshIDs;
};

#endif //OPENGLRENDERINGENGINE_MESH_NODE_HPP
