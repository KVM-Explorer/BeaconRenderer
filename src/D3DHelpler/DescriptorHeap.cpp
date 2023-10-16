#include "DescroptorHeap.h"
#include "d3dx12.h"
DescriptorHeap::DescriptorHeap(ID3D12Device *device, D3D12_DESCRIPTOR_HEAP_TYPE type, UINT descriptorNum, bool isShaderVisiable) : mDescriptorSize(device->GetDescriptorHandleIncrementSize(type))
{
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = descriptorNum;
    heapDesc.Type = type;
    heapDesc.Flags = isShaderVisiable ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    heapDesc.NodeMask = 0;

    device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&mHeap));
    mCPUHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mHeap->GetCPUDescriptorHandleForHeapStart());
    if(isShaderVisiable)
        mGPUHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mHeap->GetGPUDescriptorHandleForHeapStart());
    else
        mGPUHandle.ptr = 0;

}

uint DescriptorHeap::AddRtvDescriptor(ID3D12Device *device, ID3D12Resource *resource, D3D12_RENDER_TARGET_VIEW_DESC *rtvDesc)
{
    D3D12_RENDER_TARGET_VIEW_DESC desc;
    if (rtvDesc == nullptr) {
        desc.Format = resource->GetDesc().Format;
        desc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipSlice = 0;
        desc.Texture2D.PlaneSlice = 0;
    } else {
        desc = *rtvDesc;
    }
    auto cpuHandle = CPUHandle(mDescriptorNum);
    device->CreateRenderTargetView(resource, &desc, cpuHandle);
    return mDescriptorNum++;
}

uint DescriptorHeap::AddDsvDescriptor(ID3D12Device *device, ID3D12Resource *resource, D3D12_DEPTH_STENCIL_VIEW_DESC *dsvDesc)
{
    D3D12_DEPTH_STENCIL_VIEW_DESC desc;
    if (dsvDesc == nullptr) {
        desc.Format = resource->GetDesc().Format;
        desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipSlice = 0;
        desc.Flags = D3D12_DSV_FLAG_NONE;
    } else {
        desc = *dsvDesc;
    }
    auto cpuHandle = CPUHandle(mDescriptorNum);
    device->CreateDepthStencilView(resource, &desc, cpuHandle);
    return mDescriptorNum++;
}

uint DescriptorHeap::AddSrvDescriptor(ID3D12Device *device, ID3D12Resource *resource, D3D12_SHADER_RESOURCE_VIEW_DESC *srvDesc)
{
    D3D12_SHADER_RESOURCE_VIEW_DESC desc;
    if (srvDesc == nullptr) {
        desc.Format = resource->GetDesc().Format;
        desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipLevels = 1;
        desc.Texture2D.MostDetailedMip = 0;
        desc.Texture2D.PlaneSlice = 0;
        desc.Texture2D.ResourceMinLODClamp = 0.0f;
        desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    } else {
        desc = *srvDesc;
    }
    auto cpuHandle = CPUHandle(mDescriptorNum);
    device->CreateShaderResourceView(resource, &desc, cpuHandle);
    return mDescriptorNum++;
}

uint DescriptorHeap::AddUavDescriptor(ID3D12Device *device, ID3D12Resource *resource, D3D12_UNORDERED_ACCESS_VIEW_DESC *uavDesc)
{
    D3D12_UNORDERED_ACCESS_VIEW_DESC desc;
    if (uavDesc == nullptr) {
        desc.Format = resource->GetDesc().Format;
        desc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
        desc.Texture2D.MipSlice = 0;
        desc.Texture2D.PlaneSlice = 0;
    } else {
        desc = *uavDesc;
    }
    auto cpuHandle = CPUHandle(mDescriptorNum);
    device->CreateUnorderedAccessView(resource, nullptr, &desc, cpuHandle);
    return mDescriptorNum++;
}
