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
    void SyncCopy() const;
    void Signal3D(ID3D12CommandQueue *queue);
    void SignalCopy(ID3D12CommandQueue *queue);

    static void CreateSharedFence(ID3D12Device *device, CrossFrameResource *crossFrameResource1, CrossFrameResource *crossFrameResource2);
    void InitByMainGpu(ID3D12Device *device, uint width, uint height);
    void InitByAuxGpu(ID3D12Device *device, ID3D12Resource *backBuffer, HANDLE sharedHandle);

    void CreateMainRenderTarget(ID3D12Device *device, uint width, uint height);
    void CreateAuxRenderTarget(ID3D12Device *device, ID3D12Resource *backBuffer);

    void CreateConstantBuffer(ID3D12Device *device, uint entityCount, uint lightCount, uint materialCount);

    ID3D12Resource *GetResource(const std::string &name) const;
    CD3DX12_CPU_DESCRIPTOR_HANDLE GetRtv(const std::string &name) const;
    CD3DX12_GPU_DESCRIPTOR_HANDLE GetSrvCbvUav(const std::string &name) const;
    CD3DX12_CPU_DESCRIPTOR_HANDLE GetDsv(const std::string &name) const;

    ComPtr<ID3D12CommandAllocator> CopyCmdAllocator;
    ComPtr<ID3D12CommandList> CopyCmdList;
    ComPtr<ID3D12Fence> SharedFence;
    HANDLE SharedFenceHandle;
    ComPtr<ID3D12Resource> SharedRenderTarget;
    std::unique_ptr<FrameResource> LocalResource;

private:
    // Depth + ScreenTexture1
    void CreateSharedRenderTarget(ID3D12Device *device, uint width, uint height);
};