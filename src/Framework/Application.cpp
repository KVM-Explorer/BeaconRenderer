#include "Application.h"
#include <stdafx.h>
#include <tchar.h>

HWND Application::mHandle = nullptr;

HWND Application::GetHandle()
{
    return mHandle;
}

int Application::Run(RendererBase *renderer, HINSTANCE hInstance, int hCmdShow)
{
    WNDCLASSEX WindowsClass = {};
    WindowsClass.hInstance = hInstance;
    WindowsClass.style = CS_GLOBALCLASS;
    WindowsClass.cbClsExtra = 0;
    WindowsClass.cbWndExtra = 0;
    WindowsClass.hCursor = LoadCursor(nullptr, IDC_ARROW);
    WindowsClass.lpfnWndProc = Application::WindowProc;
    WindowsClass.lpszClassName = L"DirectX12 Class";
    WindowsClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(NULL_BRUSH));
    WindowsClass.cbSize = sizeof(WNDCLASSEX);
    RegisterClassEx(&WindowsClass);

    DWORD dwWndStyple = WS_OVERLAPPED | WS_SYSMENU;
    RECT rtWnd = {0, 0, static_cast<LONG>(renderer->GetWidth()), static_cast<LONG>(renderer->GetHeight())};
    AdjustWindowRect(&rtWnd, dwWndStyple, FALSE);

    // 计算窗口居中需要的屏幕坐标
    INT posX = (GetSystemMetrics(SM_CXSCREEN) - rtWnd.right - rtWnd.left) / 2;
    INT posY = (GetSystemMetrics(SM_CYSCREEN) - rtWnd.bottom - rtWnd.top) / 2;

    auto WindowsHandle = CreateWindow(WindowsClass.lpszClassName,
                                      renderer->GetTitle(),
                                      dwWndStyple,
                                      posX, posY,
                                      rtWnd.right - rtWnd.left,
                                      rtWnd.bottom - rtWnd.top,
                                      nullptr,
                                      nullptr,
                                      hInstance,
                                      renderer);

    mHandle = WindowsHandle;

    renderer->OnInit();

    ShowWindow(WindowsHandle, hCmdShow);
    UpdateWindow(WindowsHandle);

    MSG msg = {};
    while (msg.message != WM_QUIT) {
        // Process any messages in the queue.
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    renderer->OnDestory();


    // ComPtr<IDXGIDebug1> dxgiDebug;
    // if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug)))) {
    //     dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_ALL));
    // }

    // Return this part of the WM_QUIT message to Windows.
    return static_cast<char>(msg.wParam);
}

LRESULT CALLBACK Application::WindowProc(HWND hWnd, uint32_t message, WPARAM wParam, LPARAM lParam)
{
    RendererBase *renderer = reinterpret_cast<RendererBase *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
    switch (message) {
    case WM_CREATE: {
        // Save the DXSample* passed in to CreateWindow.
        LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
        SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
    }
        return 0;

    case WM_KEYDOWN:
        if (renderer) {
            renderer->OnKeyDown(static_cast<UINT8>(wParam));
        }
        return 0;

    case WM_PAINT:
        if (renderer) {
            renderer->OnUpdate();
            renderer->OnRender();
        }
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    // Handle any messages the switch statement didn't.
    return DefWindowProc(hWnd, message, wParam, lParam);
}