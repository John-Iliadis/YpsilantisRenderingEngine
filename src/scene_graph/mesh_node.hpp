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
    MeshNode(NodeType type, const std::string& name,
             glm::vec3 translation, glm::vec3 rotation, glm::vec3 scale,
             GraphNode* parent,
             uuid32_t modelID, std::vector<uuid32_t> meshIDs);
    ~MeshNode();

    void updateGlobalTransform() override;

    const std::vector<uuid32_t>& meshIDs() const;

private:
    std::vector<uuid32_t> mMeshIDs;
};

#endif //OPENGLRENDERINGENGINE_MESH_NODE_HPP
