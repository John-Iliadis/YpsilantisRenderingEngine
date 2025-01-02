//
// Created by Gianni on 2/01/2025.
//

#include "renderer.hpp"

Renderer::Renderer()
{
    mInstance.create();
}

Renderer::~Renderer()
{
    mInstance.destroy();
}
