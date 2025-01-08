//
// Created by Gianni on 8/01/2025.
//

#include "window.hpp"

Window::Window(uint32_t width, uint32_t height)
    : mWindow()
    , mWidth(width)
    , mHeight(height)
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    mWindow = glfwCreateWindow(mWidth, mHeight, "VulkanRenderer", nullptr, nullptr);
    check(mWindow, "Failed to create GLFW window.");

    glfwSetWindowUserPointer(mWindow, this);
    glfwSetKeyCallback(mWindow, keyCallback);
    glfwSetMouseButtonCallback(mWindow, mouseButtonCallback);
    glfwSetCursorPosCallback(mWindow, cursorPosCallback);
    glfwSetScrollCallback(mWindow, mouseScrollCallback);
    glfwSetFramebufferSizeCallback(mWindow, framebufferSizeCallback);
    glfwSetWindowIconifyCallback(mWindow, windowMinificationCallback);
}

Window::~Window()
{
    glfwTerminate();
}

void Window::pollEvents()
{
    mEventQueue.clear();
    glfwPollEvents();
}

void Window::waitEvents()
{
    glfwWaitEvents();
}

uint32_t Window::width() const
{
    return mWidth;
}

uint32_t Window::height() const
{
    return mHeight;
}

bool Window::opened() const
{
    return !glfwWindowShouldClose(mWindow);
}

Window::operator GLFWwindow*() const
{
    return mWindow;
}

void Window::keyCallback(GLFWwindow *glfwWindow, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(glfwWindow, GLFW_TRUE);
        return;
    }

    Window& window = *reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));

    Event event{};
    event.type = Event::Key;
    event.key = {
        .key = key,
        .action = action
    };

    window.mEventQueue.push_back(event);

    if (action == GLFW_PRESS || action == GLFW_RELEASE)
        Input::updateKeyState(key, action);
}

void Window::mouseButtonCallback(GLFWwindow *glfwWindow, int button, int action, int mods)
{
    Window& window = *reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));

    Event event{};
    event.type = Event::MouseButton;
    event.mouseButton = {
        .button = button,
        .action = action
    };

    window.mEventQueue.push_back(event);

    if (action == GLFW_PRESS || action == GLFW_RELEASE)
        Input::updateMouseButtonState(button, action);
}

void Window::cursorPosCallback(GLFWwindow *glfwWindow, double x, double y)
{
    Window& window = *reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));

    Event event{};
    event.type = Event::MouseMoved;
    event.mouseMove = {
        .x = x,
        .y = y
    };

    window.mEventQueue.push_back(event);

    Input::updateMousePosition(static_cast<float>(x), static_cast<float>(y));
}

void Window::mouseScrollCallback(GLFWwindow *glfwWindow, double x, double y)
{
    Window& window = *reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));

    Event event{};
    event.type = Event::MouseWheelScrolled;
    event.mouseWheel = {
        .x = x,
        .y = y
    };

    window.mEventQueue.push_back(event);
}

void Window::framebufferSizeCallback(GLFWwindow *glfwWindow, int width, int height)
{
    Window& window = *reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));

    window.mWidth = static_cast<uint32_t>(width);
    window.mHeight = static_cast<uint32_t>(height);

    Event event{};
    event.type = Event::Resized;
    event.size = {
        .width = width,
        .height = height
    };

    window.mEventQueue.push_back(event);
}

const std::vector<Event> &Window::events() const
{
    return mEventQueue;
}

void Window::windowMinificationCallback(GLFWwindow *glfwWindow, int minimized)
{
    Window& window = *reinterpret_cast<Window*>(glfwGetWindowUserPointer(glfwWindow));

    Event event{};
    event.type = minimized? Event::WindowMinimized : Event::WindowRestored;

    window.mEventQueue.push_back(event);
}
