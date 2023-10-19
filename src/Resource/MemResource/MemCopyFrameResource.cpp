#include "MemCopyFrameResource.h"

MemCopyFrameResource::~MemCopyFrameResource()
{
    ReadBackBuffer = nullptr;
    UploadBuffer = nullptr;
}
void MemCopyFrameResource::CreateReadBackResource(ID3D12Device *device, uint width, uint height, uint rowPitch)
{
    const CD3DX12_HEAP_PROPERTIES readBackHeapProperties(D3D12_HEAP_TYPE_READBACK);
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Height = 1;
    desc.Width = rowPitch * height; // Format is DXGI_FORMAT_R8G8B8A8_UNORM
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ThrowIfFailed(device->CreateCommittedResource(&readBackHeapProperties,
                                                  D3D12_HEAP_FLAG_NONE,
                                                  &desc,
                                                  D3D12_RESOURCE_STATE_COPY_DEST,
                                                  nullptr,
                                                  IID_PPV_ARGS(&ReadBackBuffer)));
    ReadBackBuffer->SetName(L"ReadBackBuffer");
}

void MemCopyFrameResource::CreateUploadResource(ID3D12Device *device, uint width, uint height, uint rowPitch)
{
    const CD3DX12_HEAP_PROPERTIES uploadHeapProperties(D3D12_HEAP_TYPE_UPLOAD);
    D3D12_RESOURCE_DESC desc = {};
    desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    desc.Width = rowPitch * height;
    desc.Height = 1;
    desc.DepthOrArraySize = 1;
    desc.MipLevels = 1;
    desc.Format = DXGI_FORMAT_UNKNOWN;
    desc.SampleDesc.Count = 1;
    desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    desc.Flags = D3D12_RESOURCE_FLAG_NONE;

    ThrowIfFailed(device->CreateCommittedResource(&uploadHeapProperties,
                                                  D3D12_HEAP_FLAG_NONE,
                                                  &desc,
                                                  D3D12_RESOURCE_STATE_GENERIC_READ,
                                                  nullptr,
                                                  IID_PPV_ARGS(&UploadBuffer)));
}

void MemCopyFrameResource::ImageToReadBackBuffer(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList, ID3D12Resource *src)
{
    auto layout = GetResourceLayout(device,src);
    if (ReadBackBuffer == nullptr)
        CreateReadBackResource(device,
                               layout.Footprint.Width,
                               layout.Footprint.Height,
                               layout.Footprint.RowPitch);

    CD3DX12_TEXTURE_COPY_LOCATION copySrc(src, 0);
    CD3DX12_TEXTURE_COPY_LOCATION copyDst(ReadBackBuffer.Get(), layout);
    layout = layout;

    cmdList->CopyTextureRegion(&copyDst, 0, 0, 0, &copySrc, nullptr);
}

void MemCopyFrameResource::DeviceBufferToDeviceBuffer(ID3D12Device *device, ID3D12Resource *res)
{
    auto layout = GetResourceLayout(device,res);
    if (UploadBuffer == nullptr)
        CreateUploadResource(device,
                             layout.Footprint.Width,
                             layout.Footprint.Height,
                             layout.Footprint.RowPitch);
    void *src = nullptr;
    void *dst = nullptr;
    D3D12_RANGE range;
    memset(&range,0,sizeof(D3D12_RANGE));
    ReadBackBuffer->Map(0, &range, reinterpret_cast<void **>(&src));
    UploadBuffer->Map(0, &range, reinterpret_cast<void **>(&dst));
    memcpy(dst, src, layout.Footprint.Height * layout.Footprint.RowPitch);
    // memcpy(data.get(), dst, layout.Footprint.Height * layout.Footprint.RowPitch);
    ReadBackBuffer->Unmap(0, nullptr);
    UploadBuffer->Unmap(0, nullptr);
}

void MemCopyFrameResource::BufferToImage(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList, ID3D12Resource *dst)
{
    auto targetDesc = dst->GetDesc();
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout = {};
    device->GetCopyableFootprints(&targetDesc, 0, 1, 0, &layout, nullptr, nullptr, nullptr);

    CD3DX12_TEXTURE_COPY_LOCATION copySrc(UploadBuffer.Get(), layout);
    CD3DX12_TEXTURE_COPY_LOCATION copyDst(dst, 0);

    cmdList->CopyTextureRegion(&copyDst, 0, 0, 0, &copySrc, nullptr);
}

D3D12_PLACED_SUBRESOURCE_FOOTPRINT MemCopyFrameResource::GetResourceLayout(ID3D12Device *device, ID3D12Resource *res)
{
    D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout = {};
    auto desc = res->GetDesc();
    device->GetCopyableFootprints(&desc, 0, 1, 0, &layout, nullptr, nullptr, nullptr);
    return layout;
}