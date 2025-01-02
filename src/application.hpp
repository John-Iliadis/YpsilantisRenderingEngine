//
// Created by Gianni on 2/01/2025.
//

#ifndef VULKANRENDERINGENGINE_APPLICATION_HPP
#define VULKANRENDERINGENGINE_APPLICATION_HPP

#include "glfw/glfw3.h"
#include "input//input.hpp"
#include "utils/utils.hpp"

class Application
{
public:
    Application();
    ~Application();
    void run();

private:
    void initializeGLFW();

private:
    GLFWwindow* mWindow;
};


#endif //VULKANRENDERINGENGINE_APPLICATION_HPP
