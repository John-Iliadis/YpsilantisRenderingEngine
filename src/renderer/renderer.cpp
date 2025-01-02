//
// Created by Gianni on 2/01/2025.
//

#include "renderer.hpp"

Renderer::Renderer()
{
    mInstance.create();
    mPhysicalDevice.create(mInstance);
    mDevice.create(mPhysicalDevice);
}

Renderer::~Renderer()
{
    mInstance.destroy();
}
