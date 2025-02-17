//
// Created by Gianni on 17/02/2025.
//

#include "timer.hpp"

Timer::Timer(bool start)
{
    if (start)
        begin();
}

void Timer::begin()
{
    mStartTime = std::chrono::high_resolution_clock::now();
}

void Timer::end()
{
    auto endTime = std::chrono::high_resolution_clock::now();
    mDuration = endTime - mStartTime;
}

double Timer::ellapsedSeconds()
{
    return mDuration.count();
}

double Timer::ellapsedMilli()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(mDuration).count();
}

double Timer::ellapsedMicro()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(mDuration).count();
}

double Timer::ellapsedNano()
{
    return std::chrono::duration_cast<std::chrono::nanoseconds>(mDuration).count();
}
