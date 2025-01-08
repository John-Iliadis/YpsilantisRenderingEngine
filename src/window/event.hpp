//
// Created by Gianni on 8/01/2025.
//

#ifndef VULKANRENDERINGENGINE_EVENT_HPP
#define VULKANRENDERINGENGINE_EVENT_HPP

class Event
{
public:
    struct SizeEvent
    {
        int width;
        int height;
    };

    struct KeyEvent
    {
        int key;
        int action;
    };

    struct MouseButtonEvent
    {
        int button;
        int action;
    };

    struct MouseWheelEvent
    {
        double x;
        double y;
    };

    struct MouseMoveEvent
    {
        double x;
        double y;
    };

    enum EventType
    {
        Resized,
        Key,
        MouseWheelScrolled,
        MouseButton,
        MouseMoved,
    };

    EventType type;

    union
    {
        SizeEvent size;
        KeyEvent key;
        MouseButtonEvent mouseButton;
        MouseWheelEvent mouseWheel;
        MouseMoveEvent mouseMove;
    };
};

#endif //VULKANRENDERINGENGINE_EVENT_HPP
