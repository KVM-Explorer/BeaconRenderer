#pragma once
#include <stdafx.h>
#include "DataStruct.h"
#include "D3DHelpler/UploadBuffer.h"
#include "D3DHelpler/DescroptorHeap.h"
#include "D3DHelpler/ResourceRegister.h"

class FrameResource {
public:
    FrameResource(ResourceRegister *resourceRegister, ID3D12Device *device);
    ~FrameResource();
    FrameResource(const FrameResource &) = delete;
    FrameResource &operator=(const FrameResource &) = delete;
    FrameResource(FrameResource &&) = default;
    FrameResource &operator=(FrameResource &&) = default;

    void Reset() const;
    void Sync() const;
    void Signal(ID3D12CommandQueue *queue);
    void Release();
    void CreateRenderTarget(ID3D12Device *device, ID3D12Resource *backBuffer);

    void CreateConstantBuffer(ID3D12Device *device, uint entityCount, uint lightCount, uint materialCount);
    void SetSceneConstant();
    ID3D12Resource *GetResource(const std::string &name) const;
    CD3DX12_CPU_DESCRIPTOR_HANDLE GetRtv(const std::string &name) const;
    CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrvCbvUav(const std::string &name) const;
    CD3DX12_CPU_DESCRIPTOR_HANDLE GetDsv(const std::string &name) const;

private:
    void Init(ID3D12Device *device);

public:

    ComPtr<ID3D12CommandAllocator> CmdAllocator;
    ComPtr<ID3D12GraphicsCommandList> CmdList;
    ComPtr<ID3D12Fence> Fence;
    uint64 FenceValue = 0;

    std::unique_ptr<UploadBuffer<SceneInfo>> SceneConstant;
    std::unique_ptr<UploadBuffer<EntityInfo>> EntityConstant;
    std::unique_ptr<UploadBuffer<Light>> LightConstant;
    std::unique_ptr<UploadBuffer<MaterialInfo>> MaterialConstant;

    // Resource
    std::vector<Texture> RenderTargets; // No SwapChain Buffer

    std::shared_ptr<DescriptorHeap> RtvDescriptorHeap;
    std::shared_ptr<DescriptorHeap> DsvDescriptorHeap;
    std::shared_ptr<DescriptorHeap> SrvCbvUavDescriptorHeap;

private:
    std::unordered_map<std::string, uint> ResourceMap;
    std::unordered_map<std::string, uint> RtvMap;
    std::unordered_map<std::string, uint> SrvCbvUavMap;
    std::unordered_map<std::string, uint> DsvMap;
};