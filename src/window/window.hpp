//
// Created by Gianni on 8/01/2025.
//

#ifndef VULKANRENDERINGENGINE_WINDOW_HPP
#define VULKANRENDERINGENGINE_WINDOW_HPP

#include <glfw/glfw3.h>
#include "../utils.hpp"
#include "input.hpp"
#include "event.hpp"

class Window
{
public:
    Window(uint32_t width, uint32_t height);
    ~Window();

    void pollEvents();
    void waitEvents();

    uint32_t width() const;
    uint32_t height() const;
    bool opened() const;

    operator GLFWwindow*() const;

    const std::vector<Event>& events() const;

    static GLFWwindow* getGLFWwindowHandle();

private:
    static void keyCallback(GLFWwindow* glfwWindow, int key, int scancode, int action, int mods);
    static void mouseButtonCallback(GLFWwindow* glfwWindow, int button, int action, int mods);
    static void cursorPosCallback(GLFWwindow* glfwWindow, double x, double y);
    static void mouseScrollCallback(GLFWwindow* glfwWindow, double x, double y);
    static void framebufferSizeCallback(GLFWwindow* glfwWindow, int width, int height);
    static void windowMinificationCallback(GLFWwindow* glfwWindow, int minimized);

private:
    static GLFWwindow* sWindow;
    std::vector<Event> mEventQueue;
    uint32_t mWidth;
    uint32_t mHeight;
};

#endif //VULKANRENDERINGENGINE_WINDOW_HPP
