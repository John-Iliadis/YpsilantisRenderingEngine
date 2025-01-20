//
// Created by Gianni on 19/01/2025.
//

#include "model.hpp"

ModelNode::ModelNode()
    : ModelNode("", glm::identity<glm::mat4>())
{
}

ModelNode::ModelNode(const std::string &name, const glm::mat4& transformation)
    : name(name)
    , transformation(transformation)
{
}
