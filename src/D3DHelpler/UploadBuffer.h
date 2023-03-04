#pragma once
#include <stdafx.h>
#include "Tools/FrameworkHelper.h"
using Microsoft::WRL::ComPtr;

template <typename T>
class UploadBuffer {
public:
    UploadBuffer(ID3D12Device *device, UINT elementCount, bool isConstantBuffer) :
        mElementByteSize(sizeof(T)),
        mIsConstantBuffer(isConstantBuffer)
    {
        if (isConstantBuffer) {
            mElementByteSize = CalculateConstantBufferByteSize(sizeof(T));
        }
        const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
        const auto desc = CD3DX12_RESOURCE_DESC::Buffer(static_cast<uint64_t>(mElementByteSize * elementCount));
        ThrowIfFailed(device->CreateCommittedResource(&heapProperties,
                                                      D3D12_HEAP_FLAG_NONE,
                                                      &desc,
                                                      D3D12_RESOURCE_STATE_GENERIC_READ,
                                                      nullptr,
                                                      IID_PPV_ARGS(&mUploadBuffer)));
        ThrowIfFailed(mUploadBuffer->Map(0, nullptr, reinterpret_cast<void **>(&mMappingData)));
    }

    UploadBuffer(const UploadBuffer &) = delete;
    UploadBuffer &operator=(const UploadBuffer &) = delete;
    ~UploadBuffer()
    {
        if (mUploadBuffer != nullptr) {
            mUploadBuffer->Unmap(0, nullptr);
        }
        mMappingData = nullptr;
    }

    ID3D12Resource *resource() const
    {
        return mUploadBuffer.Get();
    }

    void copyData(int elementIndex, const T &data)
    {
        memcpy(&mMappingData[elementIndex * mElementByteSize], &data, sizeof(T));
    }
    void copyAllData(const T* data, const size_t num)
    {
        memcpy(mMappingData,data,sizeof(T)*num);
    }
    UINT getElementSize() const {return mElementByteSize;}
private:
    ComPtr<ID3D12Resource> mUploadBuffer;
    BYTE *mMappingData = nullptr;

    UINT mElementByteSize = 0;
    bool mIsConstantBuffer = false;
};
