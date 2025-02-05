//
// Created by Gianni on 1/02/2025.
//

#ifndef VULKANRENDERINGENGINE_BOUNDING_BOX_HPP
#define VULKANRENDERINGENGINE_BOUNDING_BOX_HPP

#include <glm/glm.hpp>

struct BoundingBox
{
    glm::vec3 min;
    glm::vec3 max;

    BoundingBox()
        : min(FLT_MAX)
        , max(-FLT_MAX)
    {
    }

    void expand(const glm::vec3& point)
    {
        min = glm::min(min, point);
        max = glm::max(max, point);
    }
};

#endif //VULKANRENDERINGENGINE_BOUNDING_BOX_HPP
