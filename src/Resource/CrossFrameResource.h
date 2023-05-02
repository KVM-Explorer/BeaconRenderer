#pragma once

#include "D3DHelpler/DescroptorHeap.h"
#include "D3DHelpler/Texture.h"
#include "D3DHelpler/UploadBuffer.h"
#include "D3DHelpler/ResourceRegister.h"
#include "DataStruct.h"
#include "FrameResource.h"
#include <stdafx.h>

class CrossFrameResource {
public:
    explicit CrossFrameResource(ResourceRegister *resourceRegister, ID3D12Device *device);
    ~CrossFrameResource();
    CrossFrameResource(const CrossFrameResource &) = delete;
    CrossFrameResource &operator=(const CrossFrameResource &) = delete;
    CrossFrameResource(CrossFrameResource &&) = default;
    CrossFrameResource &operator=(CrossFrameResource &&) = default;

    void Reset3D() const;
    void Sync3D() const;
    void ResetCopy() const;
    void SyncCopy(uint value) const;
    void Signal3D(ID3D12CommandQueue *queue);
    void SignalCopy(ID3D12CommandQueue *queue);
    void Wait3D(ID3D12CommandQueue *queue) const;
    void WaitCopy(ID3D12CommandQueue *queue, uint64 fenceValue) const;

    void InitByMainGpu(ID3D12Device *device, uint width, uint height);
    void InitByAuxGpu(ID3D12Device *device, ID3D12Resource *backBuffer, HANDLE sharedHandle);

    HANDLE CreateMainRenderTarget(ID3D12Device *device, uint width, uint height);
    void CreateAuxRenderTarget(ID3D12Device *device, ID3D12Resource *backBuffer,HANDLE sharedHandle);

    void CreateConstantBuffer(ID3D12Device *device, uint entityCount, uint lightCount, uint materialCount);
    void SetSceneConstant();

    ID3D12Resource *GetResource(const std::string &name) const;

    CD3DX12_CPU_DESCRIPTOR_HANDLE GetRtv(const std::string &name) const;
    CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrvCbvUav(const std::string &name) const;
    CD3DX12_CPU_DESCRIPTOR_HANDLE GetDsv(const std::string &name) const;
    CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrvBase() const;

    ID3D12DescriptorHeap *GetRtvHeap() const;
    ID3D12DescriptorHeap *GetDsvHeap() const;
    ID3D12DescriptorHeap *GetSrvCbvUavHeap() const;

    ComPtr<ID3D12CommandAllocator> CopyCmdAllocator;
    ComPtr<ID3D12GraphicsCommandList> CopyCmdList;
    ComPtr<ID3D12GraphicsCommandList> CmdList3D;
    ComPtr<ID3D12CommandAllocator> CmdAllocator3D;
    ComPtr<ID3D12Fence> SharedFence;
    ComPtr<ID3D12Fence> Fence;
    HANDLE SharedFenceHandle;
    uint64 FenceValue;
    uint64 SharedFenceValue;

    std::unique_ptr<UploadBuffer<SceneInfo>> SceneConstant;
    std::unique_ptr<UploadBuffer<EntityInfo>> EntityConstant;
    std::unique_ptr<UploadBuffer<Light>> LightConstant;
    std::unique_ptr<UploadBuffer<MaterialInfo>> MaterialConstant;

private:
    std::shared_ptr<DescriptorHeap> mRtvHeap;
    std::shared_ptr<DescriptorHeap> mSrvCbvUavHeap;
    std::shared_ptr<DescriptorHeap> mDsvHeap;

    std::vector<Texture> mRenderTargets;

    std::unordered_map<std::string, uint> mResourceMap;
    std::unordered_map<std::string, uint> mRtvMap;
    std::unordered_map<std::string, uint> mSrvCbvUavMap;
    std::unordered_map<std::string, uint> mDsvMap;
};