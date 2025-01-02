//
// Created by Gianni on 2/01/2025.
//

#include "application.hpp"

Application::Application()
{
    initializeGLFW();
}

Application::~Application()
{
    glfwTerminate();
}

void Application::run()
{
    while (!glfwWindowShouldClose(mWindow))
    {
        glfwPollEvents();
    }
}

void Application::initializeGLFW()
{
    static constexpr int sInitialWindowWidth = 1920;
    static constexpr int sInitialWindowHeight = 1080;
    static const char* sWindowTitle = "VulkanRenderer";

    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    mWindow = glfwCreateWindow(sInitialWindowWidth, sInitialWindowHeight, sWindowTitle, nullptr, nullptr);
    check(mWindow, "Failed to create GLFW window.");

    glfwSetKeyCallback(mWindow, [] (GLFWwindow* window, int key, int scancode, int action, int mods)
    {
        std::cout << static_cast<char>(key) << '\n';
        if (action == GLFW_PRESS || action == GLFW_RELEASE)
            Input::updateKeyState(key, action);
    });

    glfwSetMouseButtonCallback(mWindow, [] (GLFWwindow* window, int button, int action, int mods)
    {
        if (action == GLFW_PRESS || action == GLFW_RELEASE)
            Input::updateMouseButtonState(button, action);
    });

    glfwSetCursorPosCallback(mWindow, [] (GLFWwindow* window, double x, double y)
    {
        Input::updateMousePosition(static_cast<float>(x), static_cast<float>(y));
    });
}
