#include "Framework/Application.h"
#include "MemCopyBeacon.h"
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    GResource::Init("config.yaml");
    auto *renderer = new MemCopyBeacon(GResource::Width, GResource::Height, L"Mem Copy Beacon ");
    Application::Run(renderer, hInstance, nCmdShow);
}