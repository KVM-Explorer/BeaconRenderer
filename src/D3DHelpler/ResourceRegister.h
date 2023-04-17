#pragma once
#include "DescroptorHeap.h"

class ResourceRegister {
public:
    ResourceRegister(ID3D12Device *device);
    ~ResourceRegister();
    ResourceRegister(const ResourceRegister &) = delete;
    ResourceRegister &operator=(const ResourceRegister &) = delete;
    ResourceRegister(ResourceRegister &&) = default;
    ResourceRegister &operator=(ResourceRegister &&) = default;

    std::shared_ptr<DescriptorHeap> RtvDescriptorHeap;
    std::shared_ptr<DescriptorHeap> DsvDescriptorHeap;
    std::shared_ptr<DescriptorHeap> SrvCbvUavDescriptorHeap;
};