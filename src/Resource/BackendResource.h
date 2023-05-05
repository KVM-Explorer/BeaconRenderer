#pragma once
#include <stdafx.h>
#include "Pass/LightPass.h"
#include "Pass/GBufferPass.h"
#include "D3DHelpler/UploadBuffer.h"
#include "DataStruct.h"
#include "D3DHelpler/ResourceRegister.h"
#include "StageFrameResource.h"

struct BackendResource {
public:
    BackendResource(IDXGIFactory *factory, IDXGIAdapter1 *adapter,uint frameCount);
    ~BackendResource();

    void CreateRenderTargets(uint width, uint height);
    uint GetDeviceID() const { return DeviceID; }

    ComPtr<ID3D12Device> Device;
    ComPtr<ID3D12CommandQueue> DirectQueue;
    ComPtr<ID3D12CommandQueue> CopyQueue;
    std::unordered_map<std::string, ComPtr<ID3D12RootSignature>> Signature;
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> PSO;

    std::unique_ptr<GBufferPass> mGBufferPass;
    std::unique_ptr<LightPass> mLightPass;

    const uint mFrameCount;
    
    std::unique_ptr<UploadBuffer<ModelVertex>> mSceneVB;
    std::unique_ptr<UploadBuffer<uint16_t>> mSceneIB;

    std::unique_ptr<ResourceRegister> mResourceRegister;

    uint mRTVDescriptorSize;
    uint mDSVDescriptorSize;
    uint mCBVSRVUAVDescriptorSize;

    std::vector<StageFrameResource> mSFR; // Frame Count

private:
    uint DeviceID;
};