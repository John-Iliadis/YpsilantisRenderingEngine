//
// Created by Gianni on 2/01/2025.
//

#include "application.hpp"

Application::Application()
{
    initializeGLFW();
    mInstance.create();
    mRenderDevice.create(mInstance);
    mSwapchain.create(mWindow, mInstance, mRenderDevice);
}

Application::~Application()
{
    mSwapchain.destroy(mInstance, mRenderDevice);
    mRenderDevice.destroy();
    mInstance.destroy();
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

    glfwSetKeyCallback(mWindow, keyCallback);
    glfwSetMouseButtonCallback(mWindow, mouseButtonCallback);
    glfwSetCursorPosCallback(mWindow, cursorPosCallback);
}

void Application::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    if (action == GLFW_PRESS || action == GLFW_RELEASE)
        Input::updateKeyState(key, action);
}

void Application::mouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    if (action == GLFW_PRESS || action == GLFW_RELEASE)
        Input::updateMouseButtonState(button, action);
}

void Application::cursorPosCallback(GLFWwindow *window, double x, double y)
{
    Input::updateMousePosition(static_cast<float>(x), static_cast<float>(y));
}
