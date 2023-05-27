#include "Framework/Application.h"
#include "StageBeacon.h"
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    GResource::Width = 640;
    GResource::Height = 640;
    auto *renderer = new StageBeacon(GResource::Width, GResource::Height, L"Beacon Renderer", 3);
    Application::Run(renderer, hInstance, nCmdShow);
}