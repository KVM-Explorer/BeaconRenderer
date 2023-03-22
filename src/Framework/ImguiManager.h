
#pragma once
#include <stdafx.h>
class ImguiManager {
public:
    ImguiManager();
    ~ImguiManager();
    ImguiManager(const ImguiManager &) = default;
    ImguiManager &operator=(const ImguiManager &) = default;
    ImguiManager(ImguiManager &&) = delete;
    ImguiManager &operator=(ImguiManager &&) = delete;

private:
    
};