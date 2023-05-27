#include "ImguiManager.h"
#include "Framework/Application.h"
#include "GlobalResource.h"

ImguiManager::ImguiManager()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableSetMousePos + ImGuiBackendFlags_HasSetMousePos; // Enable Keyboard Controls
}

ImguiManager::~ImguiManager()
{
    ImGui_ImplDX12_Shutdown();
    mUiSrvHeap = nullptr;
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
}

void ImguiManager::Init(ID3D12Device *device)
{
    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(Application::GetHandle());

    // TODO 128 Relevate with UI count
    mUiSrvHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

    ImGui_ImplDX12_Init(
        device,
        3,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        mUiSrvHeap->Resource(),
        mUiSrvHeap->CPUHandle(0),
        mUiSrvHeap->GPUHandle(0));
    ImGui_ImplDX12_CreateDeviceObjects();
}

void ImguiManager::DrawUI(ID3D12GraphicsCommandList *cmdList, ID3D12Resource *target)
{
    // Update State
    State.RenderTime = GResource::CPUTimerManager->QueryDuration("RenderTime") / 1000.0F;
    State.DrawCallTime = GResource::CPUTimerManager->QueryDuration("DrawCall") / 1000.0F;
    State.UpdateSceneTime = GResource::CPUTimerManager->QueryDuration("UpdateScene") / 1000.0F;
    State.UpdatePassTime = GResource::CPUTimerManager->QueryDuration("UpdatePass") / 1000.0F;

    // Define GUI
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    bool show_window = true;
    ImGui::Checkbox("Enable Sphere Scene", &State.EnableSphere);
    ImGui::Checkbox("Enable Model Scene", &State.EnableModel);
    ImGui::Begin("Timer Summary", &show_window); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
    ImGui::Text("FPS: %u ", State.FPSCount);
    ImGui::Text("Render Interval: %.3f ms", State.RenderTime);
    ImGui::Text("Draw Call with UI: %.2f ms", State.DrawCallTime);
    ImGui::Text("Update Scene: %.2f ms", State.UpdateSceneTime);
    ImGui::Text("Update Pass: %.2f ms", State.UpdatePassTime);

    if(GResource::config["PassInfoUI"].as<bool>())
    {
        for (auto &timer : GResource::GPUTimer->GetTimes()) {
            ImGui::Text("%s %f", timer.second.c_str(), timer.first * 1000.0F);
        }
    }


    ImGui::End();

    // Generate GUI
    ImGui::Render();

    // Render GUI To Screen
    std::array<ID3D12DescriptorHeap *, 1> ppHeaps{mUiSrvHeap->Resource()};
    cmdList->SetDescriptorHeaps(ppHeaps.size(), ppHeaps.data());
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
    // State Convert After Render Barrier
    auto endBarrier = CD3DX12_RESOURCE_BARRIER::Transition(target,
                                                           D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                           D3D12_RESOURCE_STATE_PRESENT);
    cmdList->ResourceBarrier(1, &endBarrier);
}