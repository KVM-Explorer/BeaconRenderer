#include "Framework/Application.h"
#include "CrossBeacon.h"
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    GResource::Init("config.yaml");
    auto *renderer = new CrossBeacon(GResource::Width, GResource::Height, L"Cross Beacon Renderer");
    Application::Run(renderer, hInstance, nCmdShow);
}