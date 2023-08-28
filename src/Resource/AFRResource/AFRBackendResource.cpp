#include "AFRBackendResource.h"

AFRBackendResource::AFRBackendResource(IDXGIFactory *factory, IDXGIAdapter1 *adapter, uint frameCount, std::wstring name) :
    AFRResourceBase(factory, adapter, frameCount, name)
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
    ThrowIfFailed(Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&CopyQueue)));
    CopyQueue->SetName((name + L" CopyQueue").c_str());
}

AFRBackendResource::~AFRBackendResource()
{
    mCopyHeap.Reset();
    CopyQueue.Reset();
    Release();
}
void AFRBackendResource::CreateRenderTargetByHandle(uint width, uint height, HANDLE handle)
{
    auto hr = Device->OpenSharedHandle(handle, IID_PPV_ARGS(&mCopyHeap));
    ::CloseHandle(handle);
    ThrowIfFailed(hr);

    for (uint i = 0; i < mFrameCount; i++) {
        StageFrameResource frameResource(Device.Get(), mResourceRegister.get());
        CreateRenderTarget(&frameResource, width, height);
        frameResource.CreateLightCopyHeapBuffer(Device.Get(), mCopyHeap.Get(), i, width, height, D3D12_RESOURCE_STATE_COPY_SOURCE);
        mSFR.push_back(std::move(frameResource));
    }
}

void AFRBackendResource::CreateSharedFenceByHandles(std::vector<HANDLE> &handle)
{
    if (mSFR.empty()) std::runtime_error("CreateSharedFence must be called after CreateRenderTargets");
    for (uint i = 0; i < mFrameCount; i++) {
        mSFR[i].CreateSharedFence(Device.Get(), handle[i]);
    }
}