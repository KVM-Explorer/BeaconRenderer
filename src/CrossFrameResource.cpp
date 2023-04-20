#include "CrossFrameResource.h"
#include "Pass/GBufferPass.h"

CrossFrameResource::CrossFrameResource(ResourceRegister *resourceRegister, ID3D12Device *device) :
    mRtvHeap(resourceRegister->RtvDescriptorHeap),
    mDsvHeap(resourceRegister->DsvDescriptorHeap),
    mSrvCbvUavHeap(resourceRegister->SrvCbvUavDescriptorHeap),
    FenceValue(0)
{
    ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CmdAllocator3D)));
    ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CmdAllocator3D.Get(), nullptr, IID_PPV_ARGS(&CmdList3D)));
    CmdList3D->Close();

    ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&CopyCmdAllocator)));
    ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, CopyCmdAllocator.Get(), nullptr, IID_PPV_ARGS(&CopyCmdList)));
    CopyCmdList->Close();

    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));
}

CrossFrameResource::~CrossFrameResource()
{
    CmdAllocator3D = nullptr;
    CmdList3D = nullptr;
    Fence = nullptr;
    SharedFence = nullptr;
    SharedFenceHandle = nullptr;
    CopyCmdAllocator = nullptr;
    CopyCmdList = nullptr;
    
    mRtvHeap.reset();
    mDsvHeap.reset();
    mSrvCbvUavHeap.reset();
    mRenderTargets.clear();

    SceneConstant.reset();
    EntityConstant.reset();
    MaterialConstant.reset();
    LightConstant.reset();
}

void CrossFrameResource::Reset3D() const
{
    CmdAllocator3D->Reset();
    CmdList3D->Reset(CmdAllocator3D.Get(), nullptr);
}

void CrossFrameResource::ResetCopy() const
{
    CopyCmdAllocator->Reset();
    CopyCmdList->Reset(CopyCmdAllocator.Get(), nullptr);
}

void CrossFrameResource::Sync3D() const
{
    auto tmp = Fence->GetCompletedValue();
    if (Fence->GetCompletedValue() < FenceValue) {
        HANDLE fenceEvent = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

        ThrowIfFailed(Fence->SetEventOnCompletion(FenceValue, fenceEvent));
        WaitForSingleObject(fenceEvent, INFINITE);

        CloseHandle(fenceEvent);
    }
}
void CrossFrameResource::SyncCopy(uint value) const
{
    if (SharedFence->GetCompletedValue() < value) {
        HANDLE fenceEvent = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

        ThrowIfFailed(SharedFence->SetEventOnCompletion(value, fenceEvent));
        WaitForSingleObject(fenceEvent, INFINITE);

        CloseHandle(fenceEvent);
    }
}

void CrossFrameResource::Signal3D(ID3D12CommandQueue *cmdQueue)
{
    CmdList3D->Close();
    std::array<ID3D12CommandList *, 1> taskList = {CmdList3D.Get()};
    cmdQueue->ExecuteCommandLists(taskList.size(), taskList.data());
    auto tmp = Fence->GetCompletedValue();
    cmdQueue->Signal(Fence.Get(), ++FenceValue);
}

void CrossFrameResource::SignalCopy(ID3D12CommandQueue *cmdQueue)
{
    CopyCmdList->Close();
    std::array<ID3D12CommandList *, 1> taskList = {CopyCmdList.Get()};
    cmdQueue->ExecuteCommandLists(taskList.size(), taskList.data());
    cmdQueue->Signal(SharedFence.Get(), ++FenceValue);
}

void CrossFrameResource::Wait3D(ID3D12CommandQueue *queue) const
{
    queue->Wait(Fence.Get(), FenceValue);
}

void CrossFrameResource::WaitCopy(ID3D12CommandQueue *queue, uint64 fenceValue) const
{
    queue->Wait(SharedFence.Get(), fenceValue);
}

ID3D12Resource *CrossFrameResource::GetResource(const std::string &name) const
{
    return mRenderTargets.at(mResourceMap.at(name)).Resource();
}

void CrossFrameResource::InitByMainGpu(ID3D12Device *device, uint width, uint height)
{
    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_SHARED | D3D12_FENCE_FLAG_SHARED_CROSS_ADAPTER, IID_PPV_ARGS(&SharedFence)));
    ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&CopyCmdAllocator)));
    ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, CopyCmdAllocator.Get(), nullptr, IID_PPV_ARGS(&CopyCmdList)));
    CopyCmdList->Close();

    ThrowIfFailed(device->CreateSharedHandle(SharedFence.Get(), nullptr, GENERIC_ALL, nullptr, &SharedFenceHandle));

    CreateMainRenderTarget(device, width, height);
}

void CrossFrameResource::InitByAuxGpu(ID3D12Device *device, ID3D12Resource *backBuffer, HANDLE sharedHandle)
{
    // Create Shared Fence
    HRESULT hrOpenSharedHandleResult = device->OpenSharedHandle(sharedHandle, IID_PPV_ARGS(&SharedFence));
    ::CloseHandle(sharedHandle);
    ThrowIfFailed(hrOpenSharedHandleResult);

}

HANDLE CrossFrameResource::CreateMainRenderTarget(ID3D12Device *device, uint width, uint height)
{
    ///================== Create Render Target Resource ================
    // Render Target Resource GBuffer*4
    for (uint i = 0; i < GBufferPass::GetTargetCount(); ++i) {
        Texture texture(device,
                        GBufferPass::GetTargetFormat()[i],
                        width,
                        height,
                        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        std::string index = "GBuffer" + std::to_string(mRenderTargets.size());
        mResourceMap[index] = i;
        mRenderTargets.push_back(std::move(texture));
    }

    // GBuffer Texture Rtv Srv
    for (uint i = 0; i < GBufferPass::GetTargetCount(); i++) {
        std::string index = "GBuffer" + std::to_string(i);
        mSrvCbvUavMap[index] = mSrvCbvUavHeap->AddSrvDescriptor(device, GetResource(index));
        mRtvMap[index] = mRtvHeap->AddRtvDescriptor(device, GetResource(index));
    }

    // Depth Texture Resource
    mResourceMap["Depth"] = mRenderTargets.size();
    mRenderTargets.emplace_back(device,
                                GBufferPass::GetDepthFormat(),
                                width,
                                height,
                                D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
                                true);
    // Create Depth Stencil View
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDescriptor = {};
    dsvDescriptor.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDescriptor.Flags = D3D12_DSV_FLAG_NONE;
    dsvDescriptor.Format = GBufferPass::GetDepthFormat();
    dsvDescriptor.Texture2D.MipSlice = 0;
    mDsvMap["Depth"] = mDsvHeap->AddDsvDescriptor(device, GetResource("Depth"), &dsvDescriptor);

    // Create Depth Texture SRV
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDescriptor = {};
    srvDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescriptor.Texture2D.MipLevels = 1;
    srvDescriptor.Texture2D.MostDetailedMip = 0;
    srvDescriptor.Texture2D.PlaneSlice = 0;
    srvDescriptor.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDescriptor.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    mSrvCbvUavMap["Depth"] = mSrvCbvUavHeap->AddSrvDescriptor(device, GetResource("Depth"), &srvDescriptor);

    // ScreenTexture1 Resource
    mResourceMap["ScreenTexture1"] = mRenderTargets.size();
    mRenderTargets.emplace_back(device,
                                DXGI_FORMAT_R8G8B8A8_UNORM,
                                width,
                                height,
                                D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
                                false);
    GetResource("ScreenTexture1")->SetName(L"ScreenTexture1");

    mRtvMap["ScreenTexture1"] = mRtvHeap->AddRtvDescriptor(device, GetResource("ScreenTexture1"));

    // Screen Texture1 Shared Texture
    mResourceMap["CScreenTexture1"] = mRenderTargets.size();
    mRenderTargets.emplace_back(device,
                                DXGI_FORMAT_R8G8B8A8_UNORM,
                                width,
                                height,
                                D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER,
                                false,
                                D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER | D3D12_HEAP_FLAG_SHARED);
    GetResource("CScreenTexture1")->SetName(L"CScreenTexture1");
    HANDLE sharedHandle = nullptr;
    device->CreateSharedHandle(GetResource("CScreenTexture1"), nullptr, GENERIC_ALL, nullptr, &sharedHandle);
    return sharedHandle;
}

void CrossFrameResource::CreateAuxRenderTarget(ID3D12Device *device, ID3D12Resource *backBuffer,HANDLE sharedHandle)
{
    // Screen Texture1 Shared Texture
    mResourceMap["CScreenTexture1"] = mRenderTargets.size();
    ComPtr<ID3D12Resource> resource;
    auto result = device->OpenSharedHandle(sharedHandle, IID_PPV_ARGS(&resource));
    ::CloseHandle(sharedHandle);
    ThrowIfFailed(result);
    mRenderTargets.push_back(std::move(Texture(resource.Get())));
    GetResource("CScreenTexture1")->SetName(L"CScreenTexture1");
    mSrvCbvUavMap["CScreenTexture1"] = mSrvCbvUavHeap->AddSrvDescriptor(device, GetResource("CScreenTexture1"));

    // ScreenTexture2
    mResourceMap["ScreenTexture2"] = mRenderTargets.size();
    mRenderTargets.emplace_back(device,
                                DXGI_FORMAT_R8G8B8A8_UNORM,
                                backBuffer->GetDesc().Width,
                                backBuffer->GetDesc().Height,
                                D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mSrvCbvUavMap["ScreenTexture2"] = mSrvCbvUavHeap->AddSrvDescriptor(device, GetResource("ScreenTexture2"));
    mSrvCbvUavMap["ScreenTexture2"] = mSrvCbvUavHeap->AddUavDescriptor(device, GetResource("ScreenTexture2"));

    // SwapChain Buffer
    mRenderTargets.push_back(std::move(Texture(backBuffer)));
    mResourceMap["SwapChain"] = mRenderTargets.size() - 1;
    mRtvMap["SwapChain"] = mRtvHeap->AddRtvDescriptor(device, GetResource("SwapChain"));
}

void CrossFrameResource::CreateConstantBuffer(ID3D12Device *device, uint entityCount, uint lightCount, uint materialCount)
{
    SceneConstant = std::make_unique<UploadBuffer<SceneInfo>>(device, 1, true);
    EntityConstant = std::make_unique<UploadBuffer<EntityInfo>>(device, entityCount, true);
    LightConstant = std::make_unique<UploadBuffer<Light>>(device, lightCount, false);
    MaterialConstant = std::make_unique<UploadBuffer<MaterialInfo>>(device, materialCount, false);
}

void CrossFrameResource::SetSceneConstant()
{
    CmdList3D->SetGraphicsRootConstantBufferView(1, SceneConstant->resource()->GetGPUVirtualAddress());
    CmdList3D->SetGraphicsRootShaderResourceView(2, LightConstant->resource()->GetGPUVirtualAddress());
    CmdList3D->SetGraphicsRootShaderResourceView(5, MaterialConstant->resource()->GetGPUVirtualAddress());
}

CD3DX12_CPU_DESCRIPTOR_HANDLE CrossFrameResource::GetRtv(const std::string &name) const
{
    return mRtvHeap->CPUHandle(mRtvMap.at(name));
}
CD3DX12_CPU_DESCRIPTOR_HANDLE CrossFrameResource::GetDsv(const std::string &name) const
{
    return mDsvHeap->CPUHandle(mDsvMap.at(name));
}
CD3DX12_GPU_DESCRIPTOR_HANDLE CrossFrameResource::GetSrvCbvUav(const std::string &name) const
{
    return mSrvCbvUavHeap->GPUHandle(mSrvCbvUavMap.at(name));
}

ID3D12DescriptorHeap *CrossFrameResource::GetRtvHeap() const
{
    return mRtvHeap->Resource();
}
ID3D12DescriptorHeap *CrossFrameResource::GetDsvHeap() const
{
    return mDsvHeap->Resource();
}
ID3D12DescriptorHeap *CrossFrameResource::GetSrvCbvUavHeap() const
{
    return mSrvCbvUavHeap->Resource();
}
CD3DX12_GPU_DESCRIPTOR_HANDLE CrossFrameResource::GetSrvBase() const
{
    return mSrvCbvUavHeap->GPUHandle(0);
}
