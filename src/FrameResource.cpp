#include "FrameResource.h"
#include "Pass/GBufferPass.h"
void FrameResource::Reset() const
{
    CmdAllocator->Reset();
    CmdList->Reset(CmdAllocator.Get(), nullptr);
}

void FrameResource::Sync() const
{
    if (Fence->GetCompletedValue() < FenceValue) {
        HANDLE fenceEvent = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);

        ThrowIfFailed(Fence->SetEventOnCompletion(FenceValue, fenceEvent));
        WaitForSingleObject(fenceEvent, INFINITE);

        CloseHandle(fenceEvent);
    }
}

void FrameResource::Signal(ID3D12CommandQueue *queue)
{
    CmdList->Close();
    std::array<ID3D12CommandList *, 1> taskList = {CmdList.Get()};
    queue->ExecuteCommandLists(taskList.size(), taskList.data());
    queue->Signal(Fence.Get(), ++FenceValue);
}

void FrameResource::Release()
{
    RenderTargets.clear();
    CmdList = nullptr;
    CmdAllocator = nullptr;
    Fence = nullptr;
    SceneConstant = nullptr;
    EntityConstant = nullptr;
    LightConstant = nullptr;
    RtvDescriptorHeap = nullptr;
    DsvDescriptorHeap = nullptr;
    SrvCbvUavDescriptorHeap = nullptr;
}

void FrameResource::Init(ID3D12Device *device)
{
    ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&CmdAllocator)));
    ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CmdAllocator.Get(), nullptr, IID_PPV_ARGS(&CmdList)));
    CmdList->Close();
    ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&Fence)));

    SceneConstant = std::make_unique<UploadBuffer<SceneInfo>>(device, 1, true);
    EntityConstant = std::make_unique<UploadBuffer<EntityInfo>>(device, 100, true);
    LightConstant = std::make_unique<UploadBuffer<Light>>(device, 100, true);

    RtvDescriptorHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 100);
    DsvDescriptorHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 100);
    SrvCbvUavDescriptorHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 100 ,true);

}

void FrameResource::CreateRenderTarget(ID3D12Device *device, ID3D12Resource *backBuffer)
{
    ///================== Create Render Target================
    // Render Target Resource GBuffer*4 + Depth + ScreenQuad * 2  + SwapChain Buffer
    for (uint i = 0; i < GBufferPass::GetTargetCount(); ++i) {
        Texture texture(device,
                        GBufferPass::GetTargetFormat()[i],
                        backBuffer->GetDesc().Width,
                        backBuffer->GetDesc().Height,
                        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        std::string index = "GBuffer" + std::to_string(i);
        ResourceMap[index] = i;
        RenderTargets.push_back(std::move(texture));
        
    }
    ResourceMap["Depth"] = RenderTargets.size();
    RenderTargets.emplace_back(device,
                               GBufferPass::GetDepthFormat(),
                               backBuffer->GetDesc().Width,
                               backBuffer->GetDesc().Height,
                               D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,true);

    // ScreenTexture1
    ResourceMap["ScreenTexture1"] = RenderTargets.size();
    RenderTargets.emplace_back(device,
                               DXGI_FORMAT_R8G8B8A8_UNORM,
                               backBuffer->GetDesc().Width,
                               backBuffer->GetDesc().Height,
                               D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
    // ScreenTexture2
    ResourceMap["ScreenTexture2"] = RenderTargets.size();
    RenderTargets.emplace_back(device,
                               DXGI_FORMAT_R8G8B8A8_UNORM,
                               backBuffer->GetDesc().Width,
                               backBuffer->GetDesc().Height,
                               D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);
    // SwapChain Buffer
    RenderTargets.push_back(std::move(Texture(backBuffer)));
    ResourceMap["SwapChain"] = RenderTargets.size() - 1;

    ///================== Create Render Target View================

    // Create Render Target View
    for (uint i = 0; i < GBufferPass::GetTargetCount(); i++) {
        std::string index = "GBuffer" + std::to_string(i);
        RtvMap[index] = i;
        RtvDescriptorHeap->AddRtvDescriptor(device, RenderTargets[i].Resource());
    }
    RtvMap["ScreenTexture1"] = RtvDescriptorHeap->AddRtvDescriptor(device, RenderTargets[ResourceMap["ScreenTexture1"]].Resource());

    // Create Depth Stencil View
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDescriptor = {};
    dsvDescriptor.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDescriptor.Flags = D3D12_DSV_FLAG_NONE;
    dsvDescriptor.Format = GBufferPass::GetDepthFormat();
    dsvDescriptor.Texture2D.MipSlice = 0;
    DsvDescriptorHeap->AddDsvDescriptor(device, RenderTargets[ResourceMap["Depth"]].Resource(), &dsvDescriptor);

    // SwapChain Buffer
    RtvMap["SwapChain"] = RtvDescriptorHeap->AddRtvDescriptor(device, RenderTargets[RenderTargets.size() - 1].Resource());

    ///================== Create Shader Resource View================
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDescriptor = {};
    srvDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescriptor.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    srvDescriptor.Texture2D.MipLevels = 1;
    srvDescriptor.Texture2D.MostDetailedMip = 0;
    srvDescriptor.Texture2D.PlaneSlice = 0;
    srvDescriptor.Texture2D.ResourceMinLODClamp = 0.0f;

    // GBuffer Texture Light Input SRV
    for (uint i = 0; i < GBufferPass::GetTargetCount(); i++) {
        std::string index = "GBuffer" + std::to_string(i);
        SrvCbvUavMap[index] = SrvCbvUavDescriptorHeap->AddSrvDescriptor(device, RenderTargets[i].Resource());
    }
    // Depth Texture Light Input SRV
    srvDescriptor.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
    SrvCbvUavMap["Depth"] = SrvCbvUavDescriptorHeap->AddSrvDescriptor(device, RenderTargets[ResourceMap["Depth"]].Resource(), &srvDescriptor);
    // ScreenTexture1 Sobel Input SRV
    SrvCbvUavMap["ScreenTexture1"] = SrvCbvUavDescriptorHeap->AddSrvDescriptor(device, RenderTargets[ResourceMap["ScreenTexture1"]].Resource());
    // ScreenTexture2 Sobel Output SRV
    SrvCbvUavMap["ScreenTexture2"] = SrvCbvUavDescriptorHeap->AddSrvDescriptor(device, RenderTargets[ResourceMap["ScreenTexture2"]].Resource());

    ///================== Create Unordered Access View================

    // ScreenTexture2 Sobel Output UAV
    SrvCbvUavMap["ScreenTexture2"] = SrvCbvUavDescriptorHeap->AddUavDescriptor(device, RenderTargets[ResourceMap["ScreenTexture2"]].Resource());

    /// ================== Create Depth Stencil View================
    DsvMap["Depth"] = DsvDescriptorHeap->AddDsvDescriptor(device, RenderTargets[ResourceMap["Depth"]].Resource(), &dsvDescriptor);
}

ID3D12Resource *FrameResource::GetResource(const std::string &name) const
{
    return RenderTargets[ResourceMap.at(name)].Resource();
}

CD3DX12_CPU_DESCRIPTOR_HANDLE FrameResource::GetRtv(const std::string &name) const
{
    return RtvDescriptorHeap->CPUHandle(RtvMap.at(name));
}

CD3DX12_CPU_DESCRIPTOR_HANDLE FrameResource::GetDsv(const std::string &name) const
{
    return DsvDescriptorHeap->CPUHandle(DsvMap.at(name));
}

CD3DX12_GPU_DESCRIPTOR_HANDLE FrameResource::GetSrvCbvUav(const std::string &name) const
{
    return SrvCbvUavDescriptorHeap->GPUHandle(SrvCbvUavMap.at(name));
}