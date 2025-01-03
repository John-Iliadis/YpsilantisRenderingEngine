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

bool Input::mouseButtonPressed(int button)
{
    return mMouseData.contains(button) && mMouseData.at(button) == GLFW_PRESS;
}

bool Input::mouseButtonReleased(int button)
{
    return mMouseData.contains(button) && mMouseData.at(button) == GLFW_RELEASE;
}

std::pair<float, float> Input::mousePosition()
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
