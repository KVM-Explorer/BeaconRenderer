#include "CrossFrameResource.h"
#include "Pass/GBufferPass.h"
CrossFrameResource::~CrossFrameResource()
{
    LocalResource->Reset();
    SharedFence = nullptr;
}

CrossFrameResource::CrossFrameResource(ResourceRegister *resourceRegister, ID3D12Device *device)
{
    LocalResource = std::make_unique<FrameResource>(resourceRegister);
    LocalResource->Init(device);
}

void CrossFrameResource::InitByMainGpu(ID3D12Device *device, uint width, uint height)
{
    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_SHARED | D3D12_FENCE_FLAG_SHARED_CROSS_ADAPTER, IID_PPV_ARGS(&SharedFence)));
    ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_COPY, LocalResource->CmdAllocator.Get(), nullptr, IID_PPV_ARGS(&CmdListCopy)));

    ThrowIfFailed(device->CreateSharedHandle(SharedFence.Get(), nullptr, GENERIC_ALL, nullptr, &SharedFenceHandle));

    CreateMainRenderTarget(device, width, height);
}

void CrossFrameResource::InitByAuxGpu(ID3D12Device *device, ID3D12Resource *backBuffer, HANDLE sharedHandle)
{
    // Create Shared Fence
    HRESULT hrOpenSharedHandleResult = device->OpenSharedHandle(sharedHandle, IID_PPV_ARGS(&SharedFence));
    ::CloseHandle(sharedHandle);
    ThrowIfFailed(hrOpenSharedHandleResult);

    // Create Render Target
    CreateAuxRenderTarget(device, backBuffer);
}

void CrossFrameResource::CreateMainRenderTarget(ID3D12Device *device, uint width, uint height)
{
    ///================== Create Render Target Resource ================
    // Render Target Resource GBuffer*4
    for (uint i = 0; i < GBufferPass::GetTargetCount(); ++i) {
        Texture texture(device,
                        GBufferPass::GetTargetFormat()[i],
                        width,
                        height,
                        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        std::string index = "GBuffer" + std::to_string(i);
        LocalResource->ResourceMap[index] = i;
        // texture.Resource()->SetName(std::wstring(index.begin(), index.end()).c_str());
        LocalResource->RenderTargets.push_back(std::move(texture));
    }

    // GBuffer Texture Rtv Srv
    for (uint i = 0; i < GBufferPass::GetTargetCount(); i++) {
        std::string index = "GBuffer" + std::to_string(i);
        LocalResource->SrvCbvUavMap[index] = LocalResource->SrvCbvUavDescriptorHeap->AddSrvDescriptor(device, LocalResource->RenderTargets[i].Resource());
        LocalResource->RtvMap[index] = LocalResource->RtvDescriptorHeap->AddRtvDescriptor(device, LocalResource->RenderTargets[i].Resource());
    }

    // Create Shared Resouce
    CreateSharedRenderTarget(device, width, height);

    LocalResource->RtvMap["ScreenTexture1"] = LocalResource->RtvDescriptorHeap->AddRtvDescriptor(device, LocalResource->RenderTargets[LocalResource->ResourceMap["ScreenTexture1"]].Resource());

    // Create Depth Stencil View
    /// ================== Create Depth Stencil View================
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDescriptor = {};
    dsvDescriptor.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDescriptor.Flags = D3D12_DSV_FLAG_NONE;
    dsvDescriptor.Format = GBufferPass::GetDepthFormat();
    dsvDescriptor.Texture2D.MipSlice = 0;
    LocalResource->DsvDescriptorHeap->AddDsvDescriptor(device, LocalResource->RenderTargets[LocalResource->ResourceMap["Depth"]].Resource(), &dsvDescriptor);
}

void CrossFrameResource::CreateAuxRenderTarget(ID3D12Device *device, ID3D12Resource *backBuffer)
{
    CreateSharedRenderTarget(device, backBuffer->GetDesc().Width, backBuffer->GetDesc().Height);

    // Depth Texture Light Input SRV
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDescriptor = {};
    srvDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescriptor.Texture2D.MipLevels = 1;
    srvDescriptor.Texture2D.MostDetailedMip = 0;
    srvDescriptor.Texture2D.PlaneSlice = 0;
    srvDescriptor.Texture2D.ResourceMinLODClamp = 0.0f;
    srvDescriptor.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    LocalResource->SrvCbvUavMap["Depth"] = LocalResource->SrvCbvUavDescriptorHeap->AddSrvDescriptor(device, LocalResource->RenderTargets[LocalResource->ResourceMap["Depth"]].Resource(), &srvDescriptor);
    // ScreenTexture1 Sobel Input SRV
    LocalResource->SrvCbvUavMap["ScreenTexture1"] = LocalResource->SrvCbvUavDescriptorHeap->AddSrvDescriptor(device, LocalResource->RenderTargets[LocalResource->ResourceMap["ScreenTexture1"]].Resource());

    // ScreenTexture2
    LocalResource->ResourceMap["ScreenTexture2"] = LocalResource->RenderTargets.size();
    LocalResource->RenderTargets.emplace_back(device,
                                              DXGI_FORMAT_R8G8B8A8_UNORM,
                                              backBuffer->GetDesc().Width,
                                              backBuffer->GetDesc().Height,
                                              D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    // SwapChain Buffer
    LocalResource->RenderTargets.push_back(std::move(Texture(backBuffer)));
    LocalResource->ResourceMap["SwapChain"] = LocalResource->RenderTargets.size() - 1;

    ///================== Create Render Target View================

    // SwapChain Buffer
    LocalResource->RtvMap["SwapChain"] = LocalResource->RtvDescriptorHeap->AddRtvDescriptor(device, LocalResource->RenderTargets[LocalResource->RenderTargets.size() - 1].Resource());

    ///================== Create Shader Resource View================

    // ScreenTexture2 Sobel Output SRV
    LocalResource->SrvCbvUavMap["ScreenTexture2"] = LocalResource->SrvCbvUavDescriptorHeap->AddSrvDescriptor(device, LocalResource->RenderTargets[LocalResource->ResourceMap["ScreenTexture2"]].Resource());

    ///================== Create Unordered Access View================

    // ScreenTexture2 Sobel Output UAV
    LocalResource->SrvCbvUavMap["ScreenTexture2"] = LocalResource->SrvCbvUavDescriptorHeap->AddUavDescriptor(device, LocalResource->RenderTargets[LocalResource->ResourceMap["ScreenTexture2"]].Resource());
}

void CrossFrameResource::CreateSharedRenderTarget(ID3D12Device *device, uint width, uint height)
{
    // Depth Texture Resource
    LocalResource->ResourceMap["Depth"] = LocalResource->RenderTargets.size();
    LocalResource->RenderTargets.emplace_back(device,
                                              GBufferPass::GetDepthFormat(),
                                              width,
                                              height,
                                              D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, true,
                                              D3D12_HEAP_FLAG_SHARED | D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER);

    // ScreenTexture1 Resource
    LocalResource->ResourceMap["ScreenTexture1"] = LocalResource->RenderTargets.size();
    LocalResource->RenderTargets.emplace_back(device,
                                              DXGI_FORMAT_R8G8B8A8_UNORM,
                                              width,
                                              height,
                                              D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET,
                                              false,
                                              D3D12_HEAP_FLAG_SHARED | D3D12_HEAP_FLAG_SHARED_CROSS_ADAPTER);
}
