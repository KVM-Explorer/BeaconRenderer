#include <stdafx.h>
#include "D3DHelpler/ResourceRegister.h"
#include "CrossFrameResource.h"
#include "DataStruct.h"

class DeviceResource {
public:
    DeviceResource(IDXGIFactory6 *factory, IDXGIAdapter1 *adapter, uint frameCount, Gpu type);
    DeviceResource(const DeviceResource &) = delete;
    DeviceResource(DeviceResource &&) = default;
    DeviceResource &operator=(const DeviceResource &) = delete;
    DeviceResource &operator=(DeviceResource &&) = default;
    ~DeviceResource();

    void CreateSwapChain(HWND handle, uint width, uint height, IDXGIFactory6 *factory);
    HANDLE InitFrameResource(uint width, uint height, uint frameIndex, HANDLE fenceHandle);
    void CreateRTV(uint width, uint height);

    ComPtr<ID3D12Device> Device;
    ComPtr<ID3D12CommandQueue> CmdQueue;
    ComPtr<ID3D12CommandQueue> CopyQueue;
    ComPtr<IDXGISwapChain4> SwapChain4;

    std::unique_ptr<ResourceRegister> mResourceRegister;
    std::vector<CrossFrameResource> FR;

    uint mRTVDescriptorSize;
    uint mDSVDescriptorSize;
    uint mSRVDescriptorSize;
    
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> PSO;
    std::unordered_map<std::string, ComPtr<ID3D12RootSignature>> Signature;

private:
    Gpu mDeviceType;
    uint mFrameCount;
};