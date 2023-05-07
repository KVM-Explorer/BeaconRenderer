#pragma once
#include <stdafx.h>
#include "Pass/QuadPass.h"
#include "Pass/SobelPass.h"
#include "D3DHelpler/UploadBuffer.h"
#include "D3DHelpler/ResourceRegister.h"
#include "StageFrameResource.h"
#include "Scene.h"
struct DisplayResource {
public:
    DisplayResource(IDXGIFactory *factory, IDXGIAdapter1 *adapter, uint frameCount);
    ~DisplayResource();
    void CreateSwapChain(IDXGIFactory6 *factory, HWND handle, uint width, uint height, size_t backendCount);
    void CreateRenderTargets(uint width, uint height, size_t backendCount);
    std::vector<HANDLE> CreateSharedTexture(uint width, uint height, size_t backendCount);
    std::vector<HANDLE> CreateSharedFence(size_t backendCount);
    std::tuple<StageFrameResource *, uint> GetCurrentFrame(uint backendIndex, Stage stage,uint currentBackendIndex);

    ComPtr<ID3D12Device> Device;
    ComPtr<IDXGISwapChain4> SwapChain;
    ComPtr<ID3D12CommandQueue> DirectQueue;

    std::unordered_map<std::string, ComPtr<ID3D12RootSignature>> Signature;
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> PSO;

    std::unique_ptr<QuadPass> mQuadPass;
    std::unique_ptr<SobelPass> mSobelPass;

    std::unique_ptr<UploadBuffer<ModelVertex>> mQuadVB;
    std::unique_ptr<UploadBuffer<uint16_t>> mQuadIB;
    Scene::RenderItemsMap mRenderItems;

    uint mRTVDescriptorSize;
    uint mDSVDescriptorSize;
    uint mCBVSRVUAVDescriptorSize;

    std::unique_ptr<ResourceRegister> mResourceRegister;

    const uint mFrameCount;
    std::vector<std::vector<StageFrameResource>> mSFR; // Device Count, Frame Count
private:
};