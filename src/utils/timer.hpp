//
// Created by Gianni on 17/02/2025.
//

#ifndef VULKANRENDERINGENGINE_TIMER_HPP
#define VULKANRENDERINGENGINE_TIMER_HPP

class Timer
{
public:
    Timer(bool start = true);

    void begin();
    void end();

    double ellapsedSeconds();
    double ellapsedMilli();
    double ellapsedMicro();
    double ellapsedNano();

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> mStartTime;
    std::chrono::duration<double> mDuration;
};

#endif //VULKANRENDERINGENGINE_TIMER_HPP
