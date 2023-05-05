#include "DisplayResource.h"

DisplayResource::DisplayResource(IDXGIFactory *factory, IDXGIAdapter1 *adapter, uint frameCount) :
    mFrameCount(frameCount)
{
    ThrowIfFailed(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&Device)));

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&DirectQueue)));

    mRTVDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    mDSVDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    mCBVSRVUAVDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    ComPtr<ID3D12InfoQueue> infoQueue;
    if (SUCCEEDED(Device->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);
        infoQueue->GetMuteDebugOutput();
    }

    mResourceRegister = std::make_unique<ResourceRegister>(Device.Get());
}

DisplayResource::~DisplayResource()
{
    mQuadIB = nullptr;
    mQuadVB = nullptr;
    mResourceRegister.reset();
    DirectQueue.Reset();
    Device.Reset();
}

void DisplayResource::CreateSwapChain(IDXGIFactory6 *factory, HWND handle, uint width, uint height, size_t backendCount)
{
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = backendCount * mFrameCount; // Swap chain uses double-buffering to minimize latency.
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;

    ComPtr<IDXGISwapChain1> swapChain;
    ThrowIfFailed(factory->CreateSwapChainForHwnd(DirectQueue.Get(), handle, &swapChainDesc, nullptr, nullptr, &swapChain));
    ThrowIfFailed(factory->MakeWindowAssociation(handle, DXGI_MWA_NO_ALT_ENTER));
    ThrowIfFailed(swapChain.As(&SwapChain));
}

void DisplayResource::CreateRenderTargets(uint width, uint height, size_t backendCount)
{
    auto swapBufferCount = backendCount * mFrameCount;

    for (uint i = 0; i < backendCount; i++) {
        std::vector<StageFrameResource> deviceFrames;
        for (uint j = 0; j < mFrameCount; j++) {
            ComPtr<ID3D12Resource> backBuffer;
            StageFrameResource frameResource(Device.Get(), mResourceRegister.get());
            SwapChain->GetBuffer(i * mFrameCount + j, IID_PPV_ARGS(&backBuffer));
            frameResource.CreateSobelBuffer(Device.Get(), width, height);
            frameResource.CreateSwapChain(backBuffer.Get());
            deviceFrames.push_back(std::move(frameResource));
        }
        mSFR.push_back(std::move(deviceFrames));
    }
}