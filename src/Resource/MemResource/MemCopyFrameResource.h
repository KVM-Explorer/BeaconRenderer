#pragma once

#include <stdafx.h>
#include "DataStruct.h"
#include "Resource/FrameResource.h"
#include "D3DHelpler/UploadBuffer.h "
#include "D3DHelpler/DescroptorHeap.h"
#include "D3DHelpler/ResourceRegister.h"

class MemCopyFrameResource {
public:
    MemCopyFrameResource(const MemCopyFrameResource &) = default;
    MemCopyFrameResource(MemCopyFrameResource &&) = delete;
    MemCopyFrameResource &operator=(const MemCopyFrameResource &) = default;
    MemCopyFrameResource &operator=(MemCopyFrameResource &&) = delete;
    MemCopyFrameResource() = default;
    ~MemCopyFrameResource();

    void CreateUploadResource(ID3D12Device *device, uint width, uint height, uint rowPitch);
    void CreateReadBackResource(ID3D12Device *device, uint width, uint height, uint rowPitch);

    /// Seconad GPU Device
    void DeviceBufferToDeviceBuffer(ID3D12Device *device);
    void BufferToImage(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList, ID3D12Resource *dst);
    void ImageToReadBackBuffer(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList, ID3D12Resource *src);

public:
    ComPtr<ID3D12Resource> UploadBuffer = nullptr;
    ComPtr<ID3D12Resource> ReadBackBuffer = nullptr;
    uint8_t *ReadBackBufferPtr = nullptr;
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT Layout;
    std::unique_ptr<uint8_t[]> data;
};
