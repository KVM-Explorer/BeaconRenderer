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
    // Backend Device Light(callback CreateLightTexture) + Open Light Copy Handle  
    void CreateLight2CopyTexture(ID3D12Device *device, HANDLE handle, uint width, uint height);
    // Backend Deivce Only Light
    void CreateLightTexture(ID3D12Device *device, uint width, uint height);
    // Display Device Shared Row Major Light Texture not Shared Heap
    HANDLE CreateLightCopyTexture(ID3D12Device *device, uint width, uint height);
    // Display Device Light Texture
    void CreateLightCopyHeapTexture(ID3D12Device *device, uint width, uint height);
    // Display Device Heap Resource Only Copy Buffer
    void CreateLightCopyHeapBuffer(ID3D12Device *device, ID3D12Heap *heap, uint index, uint width, uint height, D3D12_RESOURCE_STATES initalState);

    void CreateSharedFence(ID3D12Device *device, HANDLE handle);
    HANDLE CreateSharedFence(ID3D12Device *device);

    void CreateSobelBuffer(ID3D12Device *device, UINT width, UINT height);
    void CreateSwapChain(ID3D12Resource *resource);

    void CreateConstantBuffer(ID3D12Device *device, uint entityCount, uint lightCount, uint materialCount);
    void SetSceneTextureBase(uint index);

    void ResetDirect();
    void SubmitDirect(ID3D12CommandQueue *queue);
    void SignalDirect(ID3D12CommandQueue *queue);
    void FlushDirect();

    void ResetCopy();
    void SubmitCopy(ID3D12CommandQueue *queue);
    void SignalCopy(ID3D12CommandQueue *queue);
    void FlushCopy();

    void SetCurrentFrameCB();

    ID3D12Resource *GetResource(std::string name);
    CD3DX12_CPU_DESCRIPTOR_HANDLE GetRtv(std::string name);
    CD3DX12_CPU_DESCRIPTOR_HANDLE GetDsv(std::string name);
    CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrvCbvUav(std::string name);

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
    ComPtr<ID3D12Resource> mCopyBuffer;
    std::unordered_map<std::string, uint> mResourceMap;

    SceneCB mSceneCB;
    bool mSceneCBDirty;

    std::shared_ptr<DescriptorHeap> mRtvHeap;
    std::shared_ptr<DescriptorHeap> mDsvHeap;
    std::shared_ptr<DescriptorHeap> mSrvCbvUavHeap;
};