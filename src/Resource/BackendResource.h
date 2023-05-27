#pragma once
#include <stdafx.h>
#include "Pass/LightPass.h"
#include "Pass/GBufferPass.h"
#include "D3DHelpler/UploadBuffer.h"
#include "DataStruct.h"
#include "D3DHelpler/ResourceRegister.h"
#include "StageFrameResource.h"
#include "Scene.h"
#include "Tools/D3D12GpuTimer.h"

struct BackendResource {
public:
    BackendResource(IDXGIFactory *factory, IDXGIAdapter1 *adapter, uint frameCount, uint startFrameIndex);
    ~BackendResource();

    void CreateRenderTargets(uint width, uint height, std::vector<HANDLE> &handle);
    void CreateCompatibleRenderTargets(uint width, uint height, HANDLE handle);
    void CreateSharedFence(std::vector<HANDLE> &handle);
    std::tuple<StageFrameResource *, uint> GetCurrentFrame(Stage stage);
    void IncrementFrameIndex() { mCurrentFrameIndex = (mCurrentFrameIndex + 1) % mFrameCount; };
    
    ComPtr<ID3D12Device> Device;
    ComPtr<ID3D12CommandQueue> DirectQueue;
    ComPtr<ID3D12CommandQueue> CopyQueue;
    std::vector<ComPtr<ID3D12GraphicsCommandList>> BundleLists;
    ComPtr<ID3D12CommandAllocator> BundleAllocator;

    std::unordered_map<std::string, ComPtr<ID3D12RootSignature>> Signature;
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> PSO;

    std::unique_ptr<GBufferPass> mGBufferPass;
    std::unique_ptr<LightPass> mLightPass;

    const uint mFrameCount;

    std::unique_ptr<UploadBuffer<ModelVertex>> mSceneVB;
    std::unique_ptr<UploadBuffer<uint16_t>> mSceneIB;
    std::vector<Texture> mSceneTextures;
    Scene::RenderItemsMap mRenderItems;

    std::unique_ptr<ResourceRegister> mResourceRegister;

    uint mRTVDescriptorSize;
    uint mDSVDescriptorSize;
    uint mCBVSRVUAVDescriptorSize;

    std::vector<StageFrameResource> mSFR; // Frame Count

    std::unique_ptr<D3D12GpuTimer> GpuTimer;

private:
    uint mCurrentFrameIndex;
    ComPtr<ID3D12Heap> mCopyHeap;
};