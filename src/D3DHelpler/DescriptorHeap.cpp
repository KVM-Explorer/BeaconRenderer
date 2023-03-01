#include "DescroptorHeap.h"
#include "d3dx12.h"
DescriptorHeap::DescriptorHeap(ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT descriptorNum, bool isShaderVisiable) : mDescriptorSize(device->GetDescriptorHandleIncrementSize(type)) 
{
    mDesc.NumDescriptors = descriptorNum;
    mDesc.Type = type;
    mDesc.Flags = isShaderVisiable ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    mDesc.NodeMask = 0;

    device->CreateDescriptorHeap(&mDesc, IID_PPV_ARGS(&mHeap));
    mCPUHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mHeap->GetCPUDescriptorHandleForHeapStart());
    mGPUHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mHeap->GetGPUDescriptorHandleForHeapStart());

}
