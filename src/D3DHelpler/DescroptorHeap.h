#pragma once
#include <stdafx.h>
class DescriptorHeap {
public:
    DescriptorHeap(const DescriptorHeap &) = delete;
    DescriptorHeap(DescriptorHeap &&) = default;
    DescriptorHeap &operator=(const DescriptorHeap &) = delete;
    DescriptorHeap &operator=(DescriptorHeap &&) = default;

    DescriptorHeap(ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT descriptorNum, bool isShaderVisiable = false);
    UINT Length() const
    {
        return mDescriptorNum;
    }

    D3D12_CPU_DESCRIPTOR_HANDLE CPUHandle(int index)
    {
        return CD3DX12_CPU_DESCRIPTOR_HANDLE(mCPUHandle, index, mDescriptorSize);
    };
    D3D12_GPU_DESCRIPTOR_HANDLE GPUHandle(int index)
    {
        return CD3DX12_GPU_DESCRIPTOR_HANDLE(mGPUHandle, index, mDescriptorSize);
    };

    [[nodiscard]] ID3D12DescriptorHeap* Resource() const {return mHeap.Get();}

private:
    Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mHeap;
    UINT mDescriptorNum;

    D3D12_DESCRIPTOR_HEAP_DESC mDesc;
    UINT mDescriptorSize;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mCPUHandle;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mGPUHandle;
};
