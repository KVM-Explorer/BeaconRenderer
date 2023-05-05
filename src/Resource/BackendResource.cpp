#include "BackendResource.h"

BackendResource::BackendResource(IDXGIFactory *factory, IDXGIAdapter1 *adapter, uint frameCount) :
    mFrameCount(frameCount)
{
    static uint deviceID = 0;
    DeviceID = deviceID++;
    ThrowIfFailed(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&Device)));

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&DirectQueue)));
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
    ThrowIfFailed(Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&CopyQueue)));

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

BackendResource::~BackendResource()
{
    mSceneIB = nullptr;
    mSceneVB = nullptr;
    mResourceRegister.reset();
    DirectQueue.Reset();
    CopyQueue.Reset();
    Device.Reset();
}

void BackendResource::CreateRenderTargets(uint width, uint height)
{
    for (uint i = 0; i < mFrameCount; i++) {
        StageFrameResource frameResource(Device.Get(), mResourceRegister.get());
        frameResource.CreateGBuffer(Device.Get(), width, height, GBufferPass::GetTargetFormat(), GBufferPass::GetDepthFormat());
        mSFR.push_back(std::move(frameResource));
    }
}

void BackendResource::CreateSharedTexture(uint width, uint height, std::vector<HANDLE> &handle)
{
    if (mSFR.empty()) std::runtime_error("CreateSharedTexture must be called after CreateRenderTargets");
    for (uint i = 0; i < mFrameCount; i++) {
        mSFR[i].CreateLightBuffer(Device.Get(), handle[i], width, height);
    }
}