#include "Framework/Application.h"
#include "StageBeacon.h"
int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    GResource::Width = 640;
    GResource::Height = 640;
    auto *renderer = new StageBeacon(GResource::Width, GResource::Height, L"Beacon Renderer", 3);
    // try {
    Application::Run(renderer, hInstance, nCmdShow);
    // }catch(std::exception &e){
    //     MessageBoxA(nullptr, e.what(), "Error", MB_OK);
    // }
}