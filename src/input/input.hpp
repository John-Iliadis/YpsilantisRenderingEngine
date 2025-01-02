//
// Created by Gianni on 2/01/2025.
//

#ifndef VULKANRENDERINGENGINE_INPUT_HPP
#define VULKANRENDERINGENGINE_INPUT_HPP

#include <glfw/glfw3.h>
#include "../pch.hpp"

class Input
{
public:
    static bool keyPressed(int key);
    static bool keyReleased(int key);

    static bool mouseButtonPressed(int button);
    static bool mouseButtonReleased(int button);
    static std::pair<float, float> mousePosition();

    static void updateKeyState(int key, int state);
    static void updateMouseButtonState(int button, int state);
    static void updateMousePosition(float x, float y);

private:
    inline static std::unordered_map<int, int> mKeyData;
    inline static std::unordered_map<int, int> mMouseData;
    inline static std::pair<float, float> mMousePosition;
};

#endif //VULKANRENDERINGENGINE_INPUT_HPP
