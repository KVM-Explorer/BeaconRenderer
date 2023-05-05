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
    for (uint i = 0; i < targetFormat.size(); ++i) {
        Texture texture(device, targetFormat[i], width, height, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        mTexture.push_back(std::move(texture));
        mResourceMap["GBuffer" + std::to_string(i)] = mTexture.size() - 1;

        mDescriptorMap.RTV["GBuffer" + std::to_string(i)] = mRtvHeap->AddRtvDescriptor(device, mTexture.back().Resource());
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

void StageFrameResource::CreateLightBuffer(ID3D12Device *device, HANDLE handle,uint width,uint height)
{
    ComPtr<ID3D12Resource> resource;
    auto hr = device->OpenSharedHandle(handle, IID_PPV_ARGS(&resource));
    ::CloseHandle(handle);
    ThrowIfFailed(hr);

    Texture texture(resource.Get());
    mTexture.push_back(std::move(texture));
    mResourceMap["LightCopy"] = mTexture.size() - 1;

    Texture lightTexture(device, DXGI_FORMAT_R8G8B8A8_UNORM, width, height, D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
    mTexture.push_back(std::move(lightTexture));
    mResourceMap["Light"] = mTexture.size() - 1;
}

HANDLE StageFrameResource::CreateLightCopyBuffer(ID3D12Device *device, uint width, uint height)
{
    HANDLE handle;
    Texture texture(device, DXGI_FORMAT_R8G8B8A8_UNORM, width, height, D3D12_RESOURCE_FLAG_ALLOW_CROSS_ADAPTER,false,D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER | D3D12_HEAP_FLAG_SHARED);
    mTexture.push_back(std::move(texture));
    mResourceMap["LightCopy"] = mTexture.size() - 1;

    mDescriptorMap.CBVSRVUAV["LightCopy"] = mSrvCbvUavHeap->AddSrvDescriptor(device, mTexture.back().Resource());

    device->CreateSharedHandle(mTexture.back().Resource(), nullptr, GENERIC_ALL, nullptr, &handle);
    return handle;
}

void StageFrameResource::CreateSobelBuffer(ID3D12Device *device, UINT width, UINT height)
{
    Texture texture(device, DXGI_FORMAT_R8G8B8A8_UNORM, width, height, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    mTexture.push_back(std::move(texture));
    mResourceMap["Sobel"] = mTexture.size() - 1;

    mDescriptorMap.CBVSRVUAV["SobelUVA"] = mSrvCbvUavHeap->AddUavDescriptor(device, mTexture.back().Resource());
    mDescriptorMap.CBVSRVUAV["SobelSRV"] = mSrvCbvUavHeap->AddSrvDescriptor(device, mTexture.back().Resource());
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