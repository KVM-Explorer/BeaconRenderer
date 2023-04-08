#pragma once
#include <stdafx.h>
#include "DataStruct.h"
#include "D3DHelpler/UploadBuffer.h"
#include "D3DHelpler/DescroptorHeap.h"

class FrameResource {
public:
    FrameResource() = default;
    FrameResource(const FrameResource &) = default;
    FrameResource &operator=(const FrameResource &) = default;
    FrameResource(FrameResource &&) = delete;
    FrameResource &operator=(FrameResource &&) = delete;

    void Reset() const;
    void Sync() const;
    void Signal(ID3D12CommandQueue *queue);
    void Release();
    void Init(ID3D12Device *device);
    void CreateRenderTarget(ID3D12Device *device, ID3D12Resource *backBuffer);
    ID3D12Resource* GetResource(const std::string &name) const;
    CD3DX12_CPU_DESCRIPTOR_HANDLE GetRtv(const std::string &name) const;
    CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrvCbvUav(const std::string &name) const;
    CD3DX12_CPU_DESCRIPTOR_HANDLE GetDsv(const std::string &name) const;
    

    ComPtr<ID3D12CommandAllocator> CmdAllocator;
    ComPtr<ID3D12GraphicsCommandList> CmdList;
    ComPtr<ID3D12Fence> Fence;
    std::unique_ptr<UploadBuffer<SceneInfo>> SceneConstant;
    std::unique_ptr<UploadBuffer<EntityInfo>> EntityConstant;
    std::unique_ptr<UploadBuffer<Light>> LightConstant;
    uint64 FenceValue = 0;

    // Resource
    std::vector<Texture> RenderTargets; // No SwapChain Buffer
    std::unique_ptr<DescriptorHeap> RtvDescriptorHeap;
    std::unique_ptr<DescriptorHeap> DsvDescriptorHeap;
    std::unique_ptr<DescriptorHeap> SrvCbvUavDescriptorHeap;

private:
    std::unordered_map<std::string, uint> ResourceMap;
    std::unordered_map<std::string, uint> RtvMap;
    std::unordered_map<std::string, uint> SrvCbvUavMap;
    std::unordered_map<std::string, uint> DsvMap;
};