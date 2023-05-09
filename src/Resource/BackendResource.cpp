#include "BackendResource.h"

BackendResource::BackendResource(IDXGIFactory *factory, IDXGIAdapter1 *adapter, uint frameCount,uint startFrameIndex) :
    mFrameCount(frameCount),
    mCurrentFrameIndex(startFrameIndex)
{
    static uint deviceID = 0;
    ThrowIfFailed(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&Device)));

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&DirectQueue)));
    DirectQueue->SetName(L"Backend DirectQueue");
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
    ThrowIfFailed(Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&CopyQueue)));
    CopyQueue->SetName(L"Backend CopyQueue");

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
    mSFR.clear();
    mRenderItems.clear();
    mSceneTextures.clear();
    mSceneIB = nullptr;
    mSceneVB = nullptr;
    mResourceRegister.reset();
    DirectQueue.Reset();
    CopyQueue.Reset();
    Device.Reset();
}

void BackendResource::CreateRenderTargets(uint width, uint height, std::vector<HANDLE> &handle)
{
    for (uint i = 0; i < mFrameCount; i++) {
        StageFrameResource frameResource(Device.Get(), mResourceRegister.get());
        frameResource.CreateGBuffer(Device.Get(), width, height, GBufferPass::GetTargetFormat(), GBufferPass::GetDepthFormat());
        frameResource.CreateLightBuffer(Device.Get(), handle[i], width, height);
        mSFR.push_back(std::move(frameResource));
    }
}

void BackendResource::CreateSharedFence(std::vector<HANDLE> &handle)
{
    if (mSFR.empty()) std::runtime_error("CreateSharedFence must be called after CreateRenderTargets");
    for (uint i = 0; i < mFrameCount; i++) {
        mSFR[i].CreateSharedFence(Device.Get(), handle[i]);
    }
}

std::tuple<StageFrameResource *, uint> BackendResource::GetCurrentFrame(Stage stage)
{
    uint frameIndex = 0;
    switch (stage) {
    case Stage::DeferredRendering:
        return {&mSFR[mCurrentFrameIndex], mCurrentFrameIndex};
    case Stage::CopyTexture:
        frameIndex = (mCurrentFrameIndex - 1 + mFrameCount) % mFrameCount;
        return {&mSFR[frameIndex], frameIndex};
    case Stage::PostProcess:
        frameIndex = (mCurrentFrameIndex - 2 + mFrameCount) % mFrameCount;
        return {&mSFR[frameIndex], frameIndex};
    default:
        throw std::runtime_error("Stage not supported");
    }
}