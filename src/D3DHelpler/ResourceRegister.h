#pragma once
#include "DescroptorHeap.h"

class ResourceRegister {
public:
    ResourceRegister(ID3D12Device *device);
    ~ResourceRegister();
    ResourceRegister(const ResourceRegister &) = default;
    ResourceRegister &operator=(const ResourceRegister &) = default;
    ResourceRegister(ResourceRegister &&) = delete;
    ResourceRegister &operator=(ResourceRegister &&) = delete;

    std::shared_ptr<DescriptorHeap> RtvDescriptorHeap;
    std::shared_ptr<DescriptorHeap> DsvDescriptorHeap;
    std::shared_ptr<DescriptorHeap> SrvCbvUavDescriptorHeap;
};