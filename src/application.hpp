//
// Created by Gianni on 2/01/2025.
//

#ifndef VULKANRENDERINGENGINE_APPLICATION_HPP
#define VULKANRENDERINGENGINE_APPLICATION_HPP

#include <glfw/glfw3.h>
#include "vk/include.hpp"
#include "input.hpp"
#include "utils.hpp"

class Application
{
public:
    Application();
    ~Application();
    void run();

private:
    void initializeGLFW();

    static void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* window, double x, double y);

private:
    GLFWwindow* mWindow;
    VulkanInstance mInstance;
    VulkanRenderDevice mRenderDevice;
    VulkanSwapchain mSwapchain;
};

#endif //VULKANRENDERINGENGINE_APPLICATION_HPP
