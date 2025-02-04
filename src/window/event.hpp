//
// Created by Gianni on 8/01/2025.
//

#ifndef VULKANRENDERINGENGINE_EVENT_HPP
#define VULKANRENDERINGENGINE_EVENT_HPP

class Event
{
public:
    struct WindowResize
    {
        int width;
        int height;
    };

    struct Key
    {
        int key;
        int action;
    };

    struct MouseButton
    {
        int button;
        int action;
    };

    struct MouseWheel
    {
        double x;
        double y;
    };

    struct MouseMove
    {
        double x;
        double y;
    };

    std::variant<WindowResize,
        Key,
        MouseButton,
        MouseWheel,
        MouseMove> data;

    template<typename T>
    Event(const T& event) : data(event) {}

    template<typename T>
    T* getIf() { return std::get_if<T>(&data); }

    template<typename T>
    const T* getIf() const { return std::get_if<T>(&data); }
};

#endif //VULKANRENDERINGENGINE_EVENT_HPP
