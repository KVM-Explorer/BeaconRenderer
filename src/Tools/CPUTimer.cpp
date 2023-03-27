#include "CPUTimer.h"

void CPUTimer::BeginTimer(std::string name)
{
    if (!mTimerPoints.contains(name)) {
        mTimerPoints[name] = std::chrono::high_resolution_clock::now();
        mTimerState[name]++;
    } else {
        if (!mTimerState[name]) {
            mTimerPoints[name] = std::chrono::high_resolution_clock::now();
            mTimerState[name]++;
        } else {
            throw std::runtime_error("Timer is runing");
        }
    }
}

void CPUTimer::EndTimer(std::string name)
{
    if (mTimerPoints.contains(name) && mTimerState[name]) {
        auto timePoint = std::chrono::high_resolution_clock::now();
        mDuration[name] = std::chrono::duration_cast<std::chrono::milliseconds>(timePoint - mTimerPoints[name]).count();
        mTimerState[name]--;
    } else {
        throw std::runtime_error("Timer no exist or not working");
    }
}

void CPUTimer::UpdateTimer(std::string name)
{
    if (mTimerPoints.contains(name)) {
        auto timePoint = std::chrono::high_resolution_clock::now();
        mDuration[name] = std::chrono::duration_cast<std::chrono::microseconds>(timePoint - mTimerPoints[name]).count();
        mTimerPoints[name] = timePoint;
        mTimerState[name]++;
    } else {
        mTimerPoints[name] = std::chrono::high_resolution_clock::now();
    }
}

uint CPUTimer::QueryDuration(std::string name)
{
    if (mDuration.contains(name)) return mDuration[name];
    return 0;
}
