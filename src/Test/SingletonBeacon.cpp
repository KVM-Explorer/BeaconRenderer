#include "Framework/Application.h"
#include "Beacon.h"
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    GResource::Init("config.yaml");
    auto *renderer = new Beacon(GResource::Width, GResource::Height, L"Single Beacon Renderer");
    Application::Run(renderer, hInstance, nCmdShow);
}