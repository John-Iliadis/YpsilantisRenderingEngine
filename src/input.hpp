//
// Created by Gianni on 2/01/2025.
//

#ifndef VULKANRENDERINGENGINE_INPUT_HPP
#define VULKANRENDERINGENGINE_INPUT_HPP

#include <glfw/glfw3.h>
#include <glm/glm.hpp>

class Input
{
public:
    static bool keyPressed(int key);
    static bool keyReleased(int key);

    static bool mouseButtonPressed(int button);
    static bool mouseButtonReleased(int button);
    static glm::vec2 mousePosition();

    static void updateKeyState(int key, int state);
    static void updateMouseButtonState(int button, int state);
    static void updateMousePosition(float x, float y);

private:
    inline static std::unordered_map<int, int> mKeyData;
    inline static std::unordered_map<int, int> mMouseData;
    inline static glm::vec2 mMousePosition;
};

#endif //VULKANRENDERINGENGINE_INPUT_HPP
