#include "AFRResourceBase.h"

AFRResourceBase::AFRResourceBase(IDXGIFactory *factory, IDXGIAdapter1 *adapter, uint frameCount, std::wstring name) :
    mFrameCount(frameCount)
{
    ThrowIfFailed(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&Device)));

    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    ThrowIfFailed(Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&DirectQueue)));
    DirectQueue->SetName((name + L" DirectQueue").c_str());

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

std::tuple<StageFrameResource *, uint> AFRResourceBase::GetCurrentFrameResource()
{
    return {&mSFR[mCurrentFrameIndex], mCurrentFrameIndex};
}

void AFRResourceBase::IncreaseFrameIndex()
{
    mCurrentFrameIndex = (mCurrentFrameIndex + 1) % mFrameCount;
}

void AFRResourceBase::CreateRenderTarget(StageFrameResource *frameResource, uint width, uint height)
{
    frameResource->CreateGBuffer(Device.Get(), width, height, GBufferPass::GetTargetFormat(), GBufferPass::GetDepthFormat());
    frameResource->CreateLightTexture(Device.Get(), width, height);
    frameResource->CreateSobelBuffer(Device.Get(), width, height);
    frameResource->CreateQuadRtvBuffer(Device.Get(), width, height, "MixTex");
}

void AFRResourceBase::Release()
{
    mSFR.clear();
    mRenderItems.clear();
    mSceneTextures.clear();
    mSceneIB = nullptr;
    mSceneVB = nullptr;
    mQuadIB = nullptr;
    mQuadVB = nullptr;
    mResourceRegister.reset();
    DirectQueue.Reset();
    Device.Reset();
}

void AFRResourceBase::CreateScreenQuadView()
{
    ScreenQuadVBView.BufferLocation = mQuadVB->resource()->GetGPUVirtualAddress();
    ScreenQuadVBView.StrideInBytes = sizeof(ModelVertex);
    ScreenQuadVBView.SizeInBytes = sizeof(ModelVertex) * 4;

    ScreenQuadIBView.BufferLocation = mQuadIB->resource()->GetGPUVirtualAddress();
    ScreenQuadIBView.Format = DXGI_FORMAT_R32_UINT;
    ScreenQuadIBView.SizeInBytes = sizeof(uint) * 6;
}