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
    mTexture.clear();
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
    for(uint i=0;i<targetFormat.size();++i)
    {
        Texture texture(device, targetFormat[i], width, height, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        mTexture.push_back(std::move(texture));
        mResourceMap["GBuffer" + std::to_string(i)] = mTexture.size() - 1;

        mDescriptorMap.RTV["GBuffer" + std::to_string(i)] = mRtvHeap->AddRtvDescriptor(device, mTexture.back().Resource());
    }
   Texture texture(device, depthFormat, width, height, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,true);
    mTexture.push_back(std::move(texture));
    mResourceMap["Depth"] = mTexture.size() - 1;

    mDescriptorMap.DSV["Depth"] = mDsvHeap->AddDsvDescriptor(device, mTexture.back().Resource());
}

HWND StageFrameResource::CreateLightBuffer(ID3D12Device *device)
{
    return nullptr;
}

void StageFrameResource::CreateLightCopyBuffer(HWND handle)
{
}

void StageFrameResource::CreateSobelBuffer(ID3D12Device *device, UINT width, UINT height)
{
    Texture texture(device, DXGI_FORMAT_R8G8B8A8_UNORM, width, height, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mTexture.push_back(std::move(texture));
    mResourceMap["Sobel"] = mTexture.size() - 1;

    mDescriptorMap.CBVSRVUAV["Sobel"] = mSrvCbvUavHeap->AddUavDescriptor(device, mTexture.back().Resource());
}

void StageFrameResource::CreateSwapChain(ID3D12Resource *resource)
{
    ComPtr<ID3D12Device> device;
    resource->GetDevice(IID_PPV_ARGS(&device));

    Texture texture(resource);
    mTexture.push_back(std::move(texture));
    mResourceMap["SwapChain"] = mTexture.size() - 1;

    mDescriptorMap.RTV["SwapChain"] = mRtvHeap->AddRtvDescriptor(device.Get(), mTexture.back().Resource());
}