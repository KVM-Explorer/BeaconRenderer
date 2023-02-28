#pragma once
#include "RendererBase.h"
class Application {
public:
    Application();
    Application(const Application &) = delete;
    Application(Application &&) = default;
    Application &operator=(const Application &) = delete;
    Application &operator=(Application &&) = default;

    static int Run(RendererBase* renderer, HINSTANCE hInstance, int nCmdShow);
    static HWND GetHandle();

protected:
    static LRESULT CALLBACK WindowProc(HWND hWnd, uint32_t message, WPARAM wParam, LPARAM lParam);

private:
    static HWND mHandle;
};