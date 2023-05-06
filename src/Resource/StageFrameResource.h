#pragma once
#include <stdafx.h>
#include "D3DHelpler/ResourceRegister.h"
#include "DataStruct.h"

struct StageFrameResource {
    StageFrameResource(ID3D12Device *device, ResourceRegister *resourceRegister);
    ~StageFrameResource();
    StageFrameResource(const StageFrameResource &) = delete;
    StageFrameResource &operator=(const StageFrameResource &) = delete;
    StageFrameResource(StageFrameResource &&) = default;
    StageFrameResource &operator=(StageFrameResource &&) = default;

    void CreateGBuffer(ID3D12Device *device, uint width, uint height, std::vector<DXGI_FORMAT> targetFormat, DXGI_FORMAT depthFormat);
    void CreateLightBuffer(ID3D12Device *device, HANDLE handle, uint width, uint height);
    HANDLE CreateLightCopyBuffer(ID3D12Device *device, uint width, uint height);
    void CreateSobelBuffer(ID3D12Device *device, UINT width, UINT height);
    void CreateSwapChain(ID3D12Resource *resource);
    void CreateConstantBuffer(ID3D12Device *device, uint entityCount, uint lightCount, uint materialCount);

    void ResetDirect();
    void SubmitDirect(ID3D12CommandQueue *queue);
    void SignalDirect(ID3D12CommandQueue *queue);
    void FlushDirect();

    ComPtr<ID3D12CommandAllocator> DirectCmdAllocator;
    ComPtr<ID3D12GraphicsCommandList> DirectCmdList;
    ComPtr<ID3D12CommandAllocator> CopyCmdAllocator;
    ComPtr<ID3D12GraphicsCommandList> CopyCmdList;
    ComPtr<ID3D12Fence> Fence;
    ComPtr<ID3D12Fence> SharedFence;
    HANDLE SharedFenceHandle;
    uint64 FenceValue;
    uint64 SharedFenceValue;

    DescriptorMap mDescriptorMap;
    std::vector<Texture> mTexture;
    std::unordered_map<std::string, uint> mResourceMap;

    SceneCB mSceneCB;
    bool mSceneCBDirty;

    std::shared_ptr<DescriptorHeap> mRtvHeap;
    std::shared_ptr<DescriptorHeap> mDsvHeap;
    std::shared_ptr<DescriptorHeap> mSrvCbvUavHeap;
};