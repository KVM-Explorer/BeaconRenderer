#include "Framework/Application.h"
#include "AFRBeacon.h"
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    GResource::Init("config.yaml");
    auto *renderer = new AFRBeacon(GResource::Width, GResource::Height, L"AFR  Beacon Renderer", 3);
    Application::Run(renderer, hInstance, nCmdShow);
}