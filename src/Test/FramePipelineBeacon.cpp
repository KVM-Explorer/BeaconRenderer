#include "Framework/Application.h"
#include  "CrossBeacon.h"
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    GResource::Width = 640;
    GResource::Height = 640;
    CrossBeacon *renderer = new CrossBeacon(GResource::Width, GResource::Height, L"Beacon Renderer");
    Application::Run(renderer, hInstance, nCmdShow);
}