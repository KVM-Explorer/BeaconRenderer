#include "Beacon.h"
#include "Framework/Application.h"
#include "Tools/FrameworkHelper.h"
#include "DataStruct.h"
Beacon::Beacon(uint width, uint height, std::wstring title) :
    RendererBase(width, height, title),
    mViewPort(0.0F, 0.0F, static_cast<float>(width), static_cast<float>(height)),
    mScissor(0, 0, static_cast<LONG>(width), static_cast<LONG>(height))

{
}

void Beacon::OnInit(std::unique_ptr<ImguiManager> &guiContext)
{
    HWND handle = Application::GetHandle();
    mGUI = std::move(guiContext);
    CreateDevice(handle);
    CreateCommandResource();
    CreateSwapChain(handle);
    CreateFence();
    LoadScene();
    // Upload Committed Resource 0 - > 1
    mCommandList->Close();
    std::array<ID3D12CommandList *, 1> taskList = {mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(taskList.size(), taskList.data());
    SyncTask();
}

void Beacon::OnRender()
{
    int frameIndex = mSwapChain->GetCurrentBackBufferIndex();
    mCommandAllocator->Reset();
    mCommandList->Reset(mCommandAllocator.Get(), nullptr);
    mCommandList->RSSetViewports(1, &mViewPort);
    mCommandList->RSSetScissorRects(1, &mScissor);

    mScene->RenderScene(mCommandList.Get(), frameIndex);
    mScene->RenderUI(mCommandList.Get(), frameIndex);

    mCommandList->Close();
    std::array<ID3D12CommandList *, 1> taskList = {mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(taskList.size(), taskList.data());

    mSwapChain->Present(0, 0);
    SyncTask();
}

void Beacon::OnUpdate()
{
    mScene->UpdateScene();
}

void Beacon::OnDestory()
{
    mScene.reset();
}
Beacon::~Beacon()
{
}
void Beacon::OnMouseDown(WPARAM btnState, int x, int y)
{
    using DirectX::XMConvertToRadians;
    if ((btnState & MK_LBUTTON) != 0) {
        // Make each pixel correspond to a quarter of a degree.
        float dx = XMConvertToRadians(0.25f * static_cast<float>(x - MouseLastPosition.x));
        float dy = XMConvertToRadians(0.25f * static_cast<float>(y - MouseLastPosition.y));
        mScene->UpdateMouse(dx, dy);
    }
    MouseLastPosition.x = x;
    MouseLastPosition.y = y;
}

void Beacon::OnKeyDown(byte key)
{
}

void Beacon::CreateDevice(HWND handle)
{
    ComPtr<ID3D12Debug3> debugController;
    UINT dxgiFactoryFlags = 0;

    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
        // debugController->SetEnableSynchronizedCommandQueueValidation(TRUE);
        // debugController->SetEnableGPUBasedValidation(TRUE);
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }

    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&mFactory)));
    ThrowIfFailed(mFactory->MakeWindowAssociation(handle, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES));

    DXGI_ADAPTER_DESC1 adapterDesc = {};
    for (UINT i = 0; DXGI_ERROR_NOT_FOUND != mFactory->EnumAdapters1(i, &mDeviceAdapter); i++) {
        mDeviceAdapter->GetDesc1(&adapterDesc);
        if (adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) continue;
        std::wstring str = adapterDesc.Description;
        OutputDebugStringW(str.c_str());
        if (i == 2 && SUCCEEDED(D3D12CreateDevice(mDeviceAdapter.Get(), D3D_FEATURE_LEVEL_12_1, _uuidof(ID3D12Device), nullptr))) break;
    }
    ThrowIfFailed(D3D12CreateDevice(mDeviceAdapter.Get(), D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&mDevice)));

    ComPtr<ID3D12InfoQueue> infoQueue;
    if (SUCCEEDED(mDevice->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
        infoQueue->GetMuteDebugOutput();
    }
}

void Beacon::CreateCommandResource()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator)));
    ThrowIfFailed(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&mCommandList)));
    ThrowIfFailed(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));
}

void Beacon::CreateSwapChain(HWND handle)
{
    ComPtr<IDXGISwapChain1> swapchain1;
    DXGI_SWAP_CHAIN_DESC1 swapchainDesc = {};
    swapchainDesc.Width = GetWidth();
    swapchainDesc.Height = GetHeight();
    swapchainDesc.BufferCount = mFrameCount;
    swapchainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapchainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapchainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapchainDesc.SampleDesc.Count = 1;

    ThrowIfFailed(mFactory->CreateSwapChainForHwnd(mCommandQueue.Get(), handle, &swapchainDesc, nullptr, nullptr, &swapchain1));
    ThrowIfFailed(swapchain1.As(&mSwapChain));
}

void Beacon::LoadScene()
{
    // TODO add read config file
    std::wstring Path = L"C:\\Users\\Geek\\Desktop\\dst";
    auto scene = std::make_unique<Scene>(Path, L"witch");

    SceneAdapter adapater{
        .Device = mDevice.Get(),
        .CommandList = mCommandList.Get(),
        .SwapChain = mSwapChain.Get(),
        .FrameWidth = mWidth,
        .FrameHeight = mHeight,
        .FrameCount = mFrameCount};
    scene->Init(adapater);

    mScene = std::move(scene);
}

void Beacon::CreateFence()
{
    ThrowIfFailed(mDevice->CreateFence(mFenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
    mFenceValue = 0;
}

void Beacon::SyncTask()
{
    mFenceValue++;
    mCommandQueue->Signal(mFence.Get(), mFenceValue);

    if (mFence->GetCompletedValue() < mFenceValue) {
        HANDLE fenceEvent = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

        ThrowIfFailed(mFence->SetEventOnCompletion(mFenceValue, fenceEvent));
        WaitForSingleObject(fenceEvent, INFINITE);

        CloseHandle(fenceEvent);
    }
}