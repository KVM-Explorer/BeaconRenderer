#include "StageFrameResource.h"

StageFrameResource::StageFrameResource(ID3D12Device *device, ResourceRegister *resourceRegister) :
    mRtvHeap(resourceRegister->RtvDescriptorHeap),
    mDsvHeap(resourceRegister->DsvDescriptorHeap),
    mSrvCbvUavHeap(resourceRegister->SrvCbvUavDescriptorHeap),
    FenceValue(0),
    SharedFenceValue(0)
{
    ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&DirectCmdAllocator)));
    ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, DirectCmdAllocator.Get(), nullptr, IID_PPV_ARGS(&DirectCmdList)));
    DirectCmdList->Close();

    ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&CopyCmdAllocator)));
    ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, CopyCmdAllocator.Get(), nullptr, IID_PPV_ARGS(&CopyCmdList)));
    CopyCmdList->Close();

    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));
}

StageFrameResource::~StageFrameResource()
{
    mSceneCB.SceneCB = nullptr;
    mSceneCB.EntityCB = nullptr;
    mSceneCB.MaterialCB = nullptr;
    mSceneCB.LightCB = nullptr;

    mTexture.clear(); // Custom Render Tagrets
    mDescriptorMap.RTV.clear();
    mDescriptorMap.DSV.clear();
    mDescriptorMap.CBVSRVUAV.clear();
    mDescriptorMap.Sampler.clear();

    Fence = nullptr;
    DirectCmdList = nullptr;
    DirectCmdAllocator = nullptr;
    CopyCmdList = nullptr;
    CopyCmdAllocator = nullptr;
}

void StageFrameResource::CreateGBuffer(ID3D12Device *device, uint width, uint height, std::vector<DXGI_FORMAT> targetFormat, DXGI_FORMAT depthFormat)
{
    for (uint i = 0; i < targetFormat.size(); ++i) {
        Texture texture(device, targetFormat[i], width, height, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        mTexture.push_back(std::move(texture));
        auto name = std::format("GBuffer{}", i);
        mResourceMap[name] = mTexture.size() - 1;

        mDescriptorMap.RTV[name] = mRtvHeap->AddRtvDescriptor(device, mTexture.back().Resource());
        mDescriptorMap.CBVSRVUAV[name] = mSrvCbvUavHeap->AddSrvDescriptor(device, mTexture.back().Resource());
    }
    Texture texture(device, depthFormat, width, height, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, true);
    mTexture.push_back(std::move(texture));

    mResourceMap["Depth"] = mTexture.size() - 1;
    mDescriptorMap.DSV["Depth"] = mDsvHeap->AddDsvDescriptor(device, mTexture.back().Resource());

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDescriptor = {};
    srvDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescriptor.Texture2D.MipLevels = 1;
    srvDescriptor.Texture2D.MostDetailedMip = 0;
    srvDescriptor.Texture2D.PlaneSlice = 0;
    srvDescriptor.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDescriptor.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    mDescriptorMap.CBVSRVUAV["Depth"] = mSrvCbvUavHeap->AddSrvDescriptor(device, mTexture.back().Resource(), &srvDescriptor);
}

void StageFrameResource::CreateLight2CopyTexture(ID3D12Device *device, HANDLE handle, uint width, uint height)
{
    ComPtr<ID3D12Resource> resource;
    auto hr = device->OpenSharedHandle(handle, IID_PPV_ARGS(&resource));
    ::CloseHandle(handle);
    ThrowIfFailed(hr);

    Texture texture(resource.Get());
    mTexture.push_back(std::move(texture));
    mResourceMap["LightCopy"] = mTexture.size() - 1;

    CreateLightTexture(device, width, height);
}

void StageFrameResource::CreateLightTexture(ID3D12Device *device, uint width, uint height)
{
    Texture lightTexture(device,
                         DXGI_FORMAT_R8G8B8A8_UNORM,
                         width, height,
                         D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
                         false,
                         D3D12_HEAP_FLAG_NONE,
                         D3D12_RESOURCE_STATE_COMMON);
    mTexture.push_back(std::move(lightTexture));
    mResourceMap["Light"] = mTexture.size() - 1;
    mDescriptorMap.RTV["Light"] = mRtvHeap->AddRtvDescriptor(device, mTexture.back().Resource());
    mTexture.back().Resource()->SetName(L"Light");
}

HANDLE StageFrameResource::CreateLightCopyTexture(ID3D12Device *device, uint width, uint height)
{
    HANDLE handle;
    Texture texture(device, DXGI_FORMAT_R8G8B8A8_UNORM,
                    width, height,
                    D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER,
                    false,
                    D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER | D3D12_HEAP_FLAG_SHARED,
                    D3D12_RESOURCE_STATE_COMMON);
    mTexture.push_back(std::move(texture));
    mResourceMap["LightCopy"] = mTexture.size() - 1;

    mDescriptorMap.CBVSRVUAV["LightCopy"] = mSrvCbvUavHeap->AddSrvDescriptor(device, mTexture.back().Resource());

    device->CreateSharedHandle(mTexture.back().Resource(), nullptr, GENERIC_ALL, nullptr, &handle);
    return handle;
}

void StageFrameResource::CreateLightCopyHeapTexture(ID3D12Device *device, uint width, uint height)
{
    Texture texture(device, DXGI_FORMAT_R8G8B8A8_UNORM,
                    width, height,
                    D3D12_RESOURCE_FLAG_NONE,
                    false,
                    D3D12_HEAP_FLAG_NONE,
                    D3D12_RESOURCE_STATE_COMMON);
    mTexture.push_back(std::move(texture));
    mResourceMap["LightCopy"] = mTexture.size() - 1;

    mDescriptorMap.CBVSRVUAV["LightCopy"] = mSrvCbvUavHeap->AddSrvDescriptor(device, mTexture.back().Resource());
}

void StageFrameResource::CreateLightCopyHeapBuffer(ID3D12Device *device, ID3D12Heap *heap, uint index, uint width, uint height, D3D12_RESOURCE_STATES initalState)
{
    auto textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1, 1);
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT layouts;
    device->GetCopyableFootprints(&textureDesc, 0, 1, 0, &layouts, nullptr, nullptr, nullptr);

    uint64 bufferSize = UpperMemorySize(layouts.Footprint.RowPitch * layouts.Footprint.Height, D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT);

    auto bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(bufferSize, D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER);

    uint64 startAddr = bufferSize * index;
    ComPtr<ID3D12Resource> resource;
    device->CreatePlacedResource(
        heap,
        startAddr,
        &bufferDesc,
        initalState,
        nullptr,
        IID_PPV_ARGS(&resource));

    Texture texture(resource.Get());
    mTexture.push_back(std::move(texture));
    mResourceMap["LightCopyBuffer"] = mTexture.size() - 1;
}

void StageFrameResource::CreateSobelBuffer(ID3D12Device *device, UINT width, UINT height)
{
    Texture texture(device, DXGI_FORMAT_R8G8B8A8_UNORM, width, height, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mTexture.push_back(std::move(texture));
    mResourceMap["Sobel"] = mTexture.size() - 1;

    mDescriptorMap.CBVSRVUAV["SobelSRV"] = mSrvCbvUavHeap->AddSrvDescriptor(device, mTexture.back().Resource());
    mDescriptorMap.CBVSRVUAV["SobelUVA"] = mSrvCbvUavHeap->AddUavDescriptor(device, mTexture.back().Resource());
}

void StageFrameResource::CreateSwapChain(ID3D12Resource *resource)
{
    static uint index = 0;
    ComPtr<ID3D12Device> device;
    resource->GetDevice(IID_PPV_ARGS(&device));

    Texture texture(resource);
    mTexture.push_back(std::move(texture));
    mResourceMap["SwapChain"] = mTexture.size() - 1;

    mDescriptorMap.RTV["SwapChain"] = mRtvHeap->AddRtvDescriptor(device.Get(), mTexture.back().Resource());
    index++;
}

void StageFrameResource::SetSceneTextureBase(uint base)
{
    mDescriptorMap.CBVSRVUAV["SceneTextureBase"] = base;
}

void StageFrameResource::CreateConstantBuffer(ID3D12Device *device, uint entityCount, uint lightCount, uint materialCount)
{
    mSceneCB.EntityCB = std::make_unique<UploadBuffer<EntityInfo>>(device, entityCount, true);
    mSceneCB.LightCB = std::make_unique<UploadBuffer<Light>>(device, lightCount, false);
    mSceneCB.MaterialCB = std::make_unique<UploadBuffer<MaterialInfo>>(device, materialCount, false);
    mSceneCB.SceneCB = std::make_unique<UploadBuffer<SceneInfo>>(device, 1, true);
}

void StageFrameResource::CreateSharedFence(ID3D12Device *device, HANDLE handle)
{
    auto hr = device->OpenSharedHandle(handle, IID_PPV_ARGS(&SharedFence));
    ::CloseHandle(handle);
    ThrowIfFailed(hr);
}

HANDLE StageFrameResource::CreateSharedFence(ID3D12Device *device)
{
    HANDLE handle;
    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_SHARED | D3D12_FENCE_FLAG_SHARED_CROSS_ADAPTER, IID_PPV_ARGS(&SharedFence)));
    device->CreateSharedHandle(SharedFence.Get(), nullptr, GENERIC_ALL, nullptr, &handle);
    return handle;
}

void StageFrameResource::ResetDirect()
{
    DirectCmdAllocator->Reset();
    DirectCmdList->Reset(DirectCmdAllocator.Get(), nullptr);
}

void StageFrameResource::SubmitDirect(ID3D12CommandQueue *queue)
{
    DirectCmdList->Close();
    ID3D12CommandList *cmdLists[] = {DirectCmdList.Get()};
    queue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
}

void StageFrameResource::SignalDirect(ID3D12CommandQueue *queue)
{
    queue->Signal(Fence.Get(), ++FenceValue);
}

void StageFrameResource::FlushDirect()
{
    if (Fence->GetCompletedValue() < FenceValue) {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(Fence->SetEventOnCompletion(FenceValue, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

void StageFrameResource::ResetCopy()
{
    CopyCmdAllocator->Reset();
    CopyCmdList->Reset(CopyCmdAllocator.Get(), nullptr);
}

void StageFrameResource::SubmitCopy(ID3D12CommandQueue *queue)
{
    CopyCmdList->Close();
    ID3D12CommandList *cmdLists[] = {CopyCmdList.Get()};
    queue->ExecuteCommandLists(_countof(cmdLists), cmdLists);
}

void StageFrameResource::SignalCopy(ID3D12CommandQueue *queue)
{
    queue->Signal(SharedFence.Get(), ++SharedFenceValue);
}

void StageFrameResource::FlushCopy()
{
    if (SharedFence->GetCompletedValue() < SharedFenceValue) {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);
        ThrowIfFailed(SharedFence->SetEventOnCompletion(SharedFenceValue, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

ID3D12Resource *StageFrameResource::GetResource(std::string name)
{
    return mTexture.at(mResourceMap.at(name)).Resource();
}

CD3DX12_CPU_DESCRIPTOR_HANDLE StageFrameResource::GetRtv(std::string name)
{
    return mRtvHeap->CPUHandle(mDescriptorMap.RTV.at(name));
}

CD3DX12_CPU_DESCRIPTOR_HANDLE StageFrameResource::GetDsv(std::string name)
{
    return mDsvHeap->CPUHandle(mDescriptorMap.DSV.at(name));
}

CD3DX12_GPU_DESCRIPTOR_HANDLE StageFrameResource::GetSrvCbvUav(std::string name)
{
    return mSrvCbvUavHeap->GPUHandle(mDescriptorMap.CBVSRVUAV.at(name));
}

void StageFrameResource::SetCurrentFrameCB()
{
    DirectCmdList->SetGraphicsRootConstantBufferView(1, mSceneCB.SceneCB->resource()->GetGPUVirtualAddress());
    DirectCmdList->SetGraphicsRootShaderResourceView(2, mSceneCB.LightCB->resource()->GetGPUVirtualAddress());
    DirectCmdList->SetGraphicsRootShaderResourceView(5, mSceneCB.MaterialCB->resource()->GetGPUVirtualAddress());
}