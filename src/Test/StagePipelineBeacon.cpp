#include "Framework/Application.h"
#include "StageBeacon.h"
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    GResource::Init("config.yaml");
    auto *renderer = new StageBeacon(GResource::Width, GResource::Height, L"Stage Beacon Renderer", 3);
    Application::Run(renderer, hInstance, nCmdShow);
}