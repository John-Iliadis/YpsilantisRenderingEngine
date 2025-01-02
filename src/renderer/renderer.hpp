//
// Created by Gianni on 2/01/2025.
//

#ifndef VULKANRENDERINGENGINE_RENDERER_HPP
#define VULKANRENDERINGENGINE_RENDERER_HPP

#include "../pch.hpp"
#include <vulkan/vulkan.h>
#include "../vk/include.hpp"

class Renderer
{
public:
    Renderer();
    ~Renderer();

private:
    VulkanInstance mInstance;
};

#endif //VULKANRENDERINGENGINE_RENDERER_HPP
