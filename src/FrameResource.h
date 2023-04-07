#pragma once
#include <stdafx.h>
#include "DataStruct.h"
#include "D3DHelpler/UploadBuffer.h"

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

    ComPtr<ID3D12CommandAllocator> CmdAllocator;
    ComPtr<ID3D12GraphicsCommandList> CmdList;
    ComPtr<ID3D12Fence> Fence;
    std::unique_ptr<UploadBuffer<SceneInfo>> SceneConstant;
    std::unique_ptr<UploadBuffer<EntityInfo>> EntityConstant;
    std::unique_ptr<UploadBuffer<Light>> LightConstant;
    uint64 FenceValue = 0;
};