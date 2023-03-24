#include "ImguiManager.h"
#include "Framework/Application.h"

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

void ImguiManager::DrawUI(ID3D12GraphicsCommandList *cmdList,ID3D12Resource *target)
{
    // Define GUI
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    bool show_window = true;
    ImGui::Begin("Another Window", &show_window); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
    ImGui::Text("Hello from another window!");

    ImGui::End();

    bool showDemo = true;
    ImGui::ShowDemoWindow(&showDemo);

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