//
// Created by Gianni on 2/01/2025.
//

#include "application.hpp"

static constexpr int sInitialWindowWidth = 1920;
static constexpr int sInitialWindowHeight = 1080;

Application::Application()
    : mSceneCamera(glm::vec3(0.f, 0.f, -5.f), 45.f, sInitialWindowHeight, sInitialWindowHeight)
{
    initializeGLFW();
    initializeVulkan();
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
    float currentTime = glfwGetTime();

    while (!glfwWindowShouldClose(mWindow))
    {
        float dt = glfwGetTime() - currentTime;
        currentTime = glfwGetTime();

        glfwPollEvents();
        update(dt);
        render();
    }

    vkDeviceWaitIdle(mRenderDevice.device);
}

void Application::update(float dt)
{
    countFPS(dt);
    mSceneCamera.update(dt);
}

void Application::render()
{

}

void Application::initializeGLFW()
{
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    mWindow = glfwCreateWindow(sInitialWindowWidth, sInitialWindowHeight, "VulkanRenderer", nullptr, nullptr);
    check(mWindow, "Failed to create GLFW window.");

    glfwSetWindowUserPointer(mWindow, this);
    glfwSetKeyCallback(mWindow, keyCallback);
    glfwSetMouseButtonCallback(mWindow, mouseButtonCallback);
    glfwSetCursorPosCallback(mWindow, cursorPosCallback);
    glfwSetScrollCallback(mWindow, mouseScrollCallback);
}

void Application::initializeVulkan()
{
    mInstance.create();
    mRenderDevice.create(mInstance);
    mSwapchain.create(mWindow, mInstance, mRenderDevice);
    createUBOs();
}

void Application::createUBOs()
{
    mViewProjectionUBOs.resize(VulkanSwapchain::swapchainImageCount());

    VkBufferUsageFlags usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;

    VkMemoryPropertyFlags memoryProperties {
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
    };

    for (VulkanBuffer& buffer : mViewProjectionUBOs)
    {
        buffer = createBuffer(mRenderDevice, sizeof(glm::mat4), usage, memoryProperties);
    }
}

void Application::countFPS(float dt)
{
    static float frameCount = 0;
    static float accumulatedTime = 0.f;

    ++frameCount;
    accumulatedTime += dt;

    if (accumulatedTime >= 1.f)
    {
        debugLog(std::format("FPS: {}", frameCount));

        frameCount = 0;
        accumulatedTime = 0.f;
    }
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

void Application::mouseScrollCallback(GLFWwindow *window, double x, double y)
{
    Application& app = *reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
    app.mSceneCamera.scroll(static_cast<float>(x), static_cast<float>(y));
}
