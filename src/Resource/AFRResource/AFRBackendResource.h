#pragma once
#include "AFRDisplayResource.h"
class AFRBackendResource : public AFRResourceBase {
public:
    AFRBackendResource(IDXGIFactory *factory, IDXGIAdapter1 *adapter, uint frameCount, std::wstring name);
    ~AFRBackendResource();

    
    void CreateRenderTargetByHandle(uint width, uint height, HANDLE handle);
    void CreateSharedFenceByHandles(std::vector<HANDLE> &handle);

    ComPtr<ID3D12CommandQueue> CopyQueue;

private:
    ComPtr<ID3D12Heap> mCopyHeap;

};
