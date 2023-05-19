#include <stdafx.h>

using TimePoint = std::chrono::high_resolution_clock::time_point;
class CPUTimer {
public:
    CPUTimer() = default;
    CPUTimer(const CPUTimer &) = default;
    CPUTimer &operator=(const CPUTimer &) = default;
    CPUTimer(CPUTimer &&) = delete;
    CPUTimer &operator=(CPUTimer &&) = delete;

    uint GetStaticTimeDuration(std::string name,TimePoint time);
    void SetStaticTime(std::string name,TimePoint time);
    void BeginTimer(std::string name);
    void EndTimer(std::string name);
    void UpdateTimer(std::string name);
    uint QueryDuration(std::string name);

private:
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> mTimerPoints;
    std::unordered_map<std::string, uint> mTimerState;
    std::unordered_map<std::string, uint> mDuration;
    std::unordered_map<std::string, uint> mTotalTime;
};