#pragma once
#include <stdafx.h>
#include "Pass/QuadPass.h"
#include "Pass/SobelPass.h"
#include "D3DHelpler/UploadBuffer.h"
#include "D3DHelpler/ResourceRegister.h"
struct DisplayResource {
public:
    DisplayResource(IDXGIFactory *factory, IDXGIAdapter1 *adapter);
    ~DisplayResource();
    void CreateSwapChain(IDXGIFactory *factory, HWND handle, uint width, uint height,size_t backendCount);


    ComPtr<ID3D12Device> Device;
    ComPtr<IDXGISwapChain4> SwapChain;
    ComPtr<ID3D12CommandQueue> RenderQueue;
    

    std::unordered_map<std::string, ComPtr<ID3D12RootSignature>> Signature;
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> PSO;

    std::unique_ptr<QuadPass> mQuadPass;
    std::unique_ptr<SobelPass> mSobelPass;


    std::unique_ptr<UploadBuffer<ModelVertex>> mQuadVB;
    std::unique_ptr<UploadBuffer<uint16_t>> mQuadIB;

    uint mRTVDescriptorSize;
    uint mDSVDescriptorSize;
    uint mCBVSRVUAVDescriptorSize;

    std::unique_ptr<ResourceRegister> mResourceRegister;
};