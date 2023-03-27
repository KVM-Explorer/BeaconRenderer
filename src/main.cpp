#include "Framework/Application.h"
#include "Beacon.h"
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    GResource::Width = 800;
    GResource::Height = 800;
    Beacon *renderer = new Beacon(GResource::Width, GResource::Height, L"Beacon Renderer");
    Application::Run(renderer, hInstance, nCmdShow);
}