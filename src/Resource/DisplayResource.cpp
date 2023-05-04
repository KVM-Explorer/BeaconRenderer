#include "DisplayResource.h"

DisplayResource::DisplayResource(IDXGIFactory *factory, IDXGIAdapter1 *adapter)
{
    ThrowIfFailed(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&Device)));

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&RenderQueue)));

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
    RenderQueue.Reset();
    Device.Reset();
}

void DisplayResource::CreateSwapChain(IDXGIFactory *factory, HWND handle, uint width, uint height,size_t backendCount)
{
}