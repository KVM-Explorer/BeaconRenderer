#include "CrossBeacon.h"
#include "Framework/Application.h"
#include <pix3/pix3.h>
#include <iostream>

CrossBeacon::CrossBeacon(uint width, uint height, std::wstring title) :
    RendererBase(width, height, title),
    mViewPort(0.0F, 0.0F, static_cast<float>(width), static_cast<float>(height)),
    mScissor(0, 0, static_cast<LONG>(width), static_cast<LONG>(height))
{
    mCrossDevice.resize(2);
    mCrossResourceRegister.resize(2);
    mCFR.resize(2);
}

CrossBeacon::~CrossBeacon()
{
    mCFR.clear();
    mCrossResourceRegister.clear();
    mCrossDevice.clear();
    mScene = nullptr;
    mSwapChain = nullptr;
    mCommandQueue = nullptr;
    mFactory = nullptr;
    mPSO.clear();
    mSignature.clear();
    mInputLayout.clear();
}

void CrossBeacon::OnUpdate()
{
    // TODO update scene constant
}

void CrossBeacon::OnRender()
{
    // TODO render scene
}

void CrossBeacon::OnInit()
{
    HWND handle = Application::GetHandle();
    CreateDevice(handle);
    CreateCommandQueue();
    CreateSwapChain(handle);

    mCrossResourceRegister[MainGpu] = std::make_unique<ResourceRegister>(mCrossDevice[MainGpu].Get());
    mCrossResourceRegister[AuxGpu] = std::make_unique<ResourceRegister>(mCrossDevice[AuxGpu].Get());

    for (uint i = 0; i < mFrameCount; ++i) {
        CrossFrameResource resource(mCrossResourceRegister[MainGpu].get(), mCrossDevice[MainGpu].Get());
        resource.InitByMainGpu(mCrossDevice[MainGpu].Get(), GetWidth(), GetHeight());
        mCFR[MainGpu].push_back(std::move(resource));
    }

    for (uint i = 0; i < mFrameCount; ++i) {
        CrossFrameResource resource(mCrossResourceRegister[AuxGpu].get(), mCrossDevice[AuxGpu].Get());
        ComPtr<ID3D12Resource> swapChainBuffer;
        mSwapChain->GetBuffer(i, IID_PPV_ARGS(&swapChainBuffer));
        resource.InitByAuxGpu(mCrossDevice[AuxGpu].Get(), swapChainBuffer.Get(), mCFR[MainGpu][i].SharedFenceHandle);
        mCFR[AuxGpu].push_back(std::move(resource));
    }
}

void CrossBeacon::OnKeyDown(byte key)
{}

void CrossBeacon::OnMouseDown(WPARAM btnState, int x, int y)
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

void CrossBeacon::OnDestory()
{}

int CrossBeacon::GetCurrentBackBuffer()
{
    return mCurrentBackBuffer = (mCurrentBackBuffer + 1) % 3;
}

void CrossBeacon::CreateDevice(HWND handle)
{
    ComPtr<ID3D12Debug3> debugController;
    ComPtr<IDXGIAdapter1> adapter;
    ComPtr<IDXGIOutput> output;
    UINT dxgiFactoryFlags = 0;

    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
        debugController->EnableDebugLayer();
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    }

    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&mFactory)));
    ThrowIfFailed(mFactory->MakeWindowAssociation(handle, DXGI_MWA_NO_ALT_ENTER | DXGI_MWA_NO_WINDOW_CHANGES));

    DXGI_ADAPTER_DESC1 adapterDesc = {};
    for (UINT i = 0; DXGI_ERROR_NOT_FOUND != mFactory->EnumAdapters1(i, &adapter); i++) {
        adapter->GetDesc1(&adapterDesc);
        if (adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE) continue;
        std::wstring str = adapterDesc.Description;
        
        auto hr = adapter->EnumOutputs(0, &output);
        if (SUCCEEDED(hr) && output != nullptr) {
            auto outputStr = std::format(L"iGPU:\n\tIndex: {} DeviceName: {}\n", i, str);
            OutputDebugStringW(outputStr.c_str());
            ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mDevice)));
        } else {
            auto outputStr = std::format(L"dGPU:\n\tIndex: {} DeviceName: {}\n", i, str);
            OutputDebugStringW(outputStr.c_str());
            ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&mCrossDevice[MainGpu])));
        }
    }

    ComPtr<ID3D12InfoQueue> infoQueue;
    if (SUCCEEDED(mCrossDevice[MainGpu]->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
        infoQueue->GetMuteDebugOutput();
    }
    if (SUCCEEDED(mCrossDevice[AuxGpu]->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
        infoQueue->GetMuteDebugOutput();
    }
}

void CrossBeacon::CreateCommandQueue()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(mCrossDevice[MainGpu]->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));
    ThrowIfFailed(mCrossDevice[AuxGpu]->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));
}

void CrossBeacon::CompileShaders()
{
}

void CrossBeacon::CreateRTV(ID3D12Device *device, IDXGISwapChain4 *swapchain, uint frameCount)
{
    // TODO create rtv
}

void CrossBeacon::CreateSwapChain(HWND handle)
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

void CrossBeacon::CreateSignature2PSO()
{
    // TODO create signature and pso
}

void CrossBeacon::CreatePass()
{
    // TODO create pass
}

void CrossBeacon::LoadScene()
{
    // TODO load scene
}

void CrossBeacon::SetPass(uint frameIndex)
{
    // TODO set pass
}

void CrossBeacon::ExecutePass(uint frameIndex)
{
    // TODO execute pass
}
