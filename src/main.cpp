#include "Framework/Application.h"
#include "Beacon.h"
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    Beacon* renderer = new Beacon(800, 600, L"Beacon Renderer");
    Application::Run(renderer, hInstance, nCmdShow);
}