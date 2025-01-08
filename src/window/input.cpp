//
// Created by Gianni on 2/01/2025.
//

#include "input.hpp"

bool Input::keyPressed(int key)
{
    return mKeyData.contains(key) && mKeyData.at(key) == GLFW_PRESS;
}

bool Input::keyReleased(int key)
{
    return mKeyData.contains(key) && mKeyData.at(key) == GLFW_RELEASE;
}

bool Input::keyPressedCtrl(int key)
{
    return keyPressed(key) && (keyPressed(GLFW_KEY_LEFT_CONTROL) || keyReleased(GLFW_KEY_RIGHT_CONTROL));
}

bool Input::keyPressedShift(int key)
{
    return keyPressed(key) && (keyPressed(GLFW_KEY_LEFT_SHIFT) || keyReleased(GLFW_KEY_RIGHT_SHIFT));
}

bool Input::mouseButtonPressed(int button)
{
    return mMouseData.contains(button) && mMouseData.at(button) == GLFW_PRESS;
}

bool Input::mouseButtonReleased(int button)
{
    return mMouseData.contains(button) && mMouseData.at(button) == GLFW_RELEASE;
}

glm::vec2 Input::mousePosition()
{
    return mMousePosition;
}

void Input::updateKeyState(int key, int state)
{
    mKeyData[key] = state;
}

void Input::updateMouseButtonState(int button, int state)
{
    mMouseData[button] = state;
}

void Input::updateMousePosition(float x, float y)
{
    mMousePosition = {x, y};
}
