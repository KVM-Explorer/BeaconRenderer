#include "ResourceRegister.h"

ResourceRegister::ResourceRegister(ID3D12Device *device)
{
    RtvDescriptorHeap = std::make_shared<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 100);
    DsvDescriptorHeap = std::make_shared<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 100);
    SrvCbvUavDescriptorHeap = std::make_shared<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 100,true);
}

ResourceRegister::~ResourceRegister()
{
    RtvDescriptorHeap.reset();
    DsvDescriptorHeap.reset();
    SrvCbvUavDescriptorHeap.reset();
}

