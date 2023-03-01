#include "DefaultBuffer.h"
#include "Tools/FrameworkHelper.h"

DefaultBuffer::DefaultBuffer(ID3D12Device *device, ID3D12GraphicsCommandList *commandList, const void *data, UINT64 byteSize)
{
    const auto resourceDesc = CD3DX12_RESOURCE_DESC::Buffer(byteSize);
    auto heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
    ThrowIfFailed(device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_COMMON,
        nullptr,
        IID_PPV_ARGS(&mDefaultBuffer)));

    heapProps = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

    ThrowIfFailed(device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&mUploadBuffer)));

    D3D12_SUBRESOURCE_DATA subResourceData = {};
    subResourceData.pData = data;
    subResourceData.RowPitch = byteSize;
    subResourceData.SlicePitch = subResourceData.RowPitch;

    auto beginBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mDefaultBuffer.Get(),
                                                             D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
    auto endBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mDefaultBuffer.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ);
    
    commandList->ResourceBarrier(1, &beginBarrier);
    UpdateSubresources<1>(commandList, mDefaultBuffer.Get(), mUploadBuffer.Get(), 0, 0, 1, &subResourceData);
    commandList->ResourceBarrier(1, &endBarrier);
    
}

void DefaultBuffer::ReleaseUploader()
{
    if (mIsMap) {
        mUploadBuffer->Unmap(0, nullptr);
        mIsMap = false;
    }
}
DefaultBuffer::~DefaultBuffer()
{
    ReleaseUploader();
}