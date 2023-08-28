#pragma once
#include "stdafx.h"
#include "Pass/Pass.h"
#include "D3DHelpler/ResourceRegister.h"
#include "Scene.h"
#include "../StageFrameResource.h"
class AFRResourceBase {
public:
    AFRResourceBase(IDXGIFactory *factory, IDXGIAdapter1 *adapter, uint frameCount, std::wstring name);
    ~AFRResourceBase(){};

    void Release();
    void CreateRenderTarget(StageFrameResource *frameResource, uint width, uint height);
    std::tuple<StageFrameResource *, uint> GetCurrentFrameResource();
    void IncreaseFrameIndex();

    ComPtr<ID3D12Device> Device;
    ComPtr<ID3D12CommandQueue> DirectQueue;

    std::unordered_map<std::string, ComPtr<ID3D12RootSignature>> Signature;
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> PSO;

    std::unique_ptr<GBufferPass> mGBufferPass;
    std::unique_ptr<LightPass> mLightPass;
    std::unique_ptr<SobelPass> mSobelPass;
    std::unique_ptr<MixQuadPass> mMixPass;

    const uint mFrameCount;

    std::unique_ptr<UploadBuffer<ModelVertex>> mSceneVB;
    std::unique_ptr<UploadBuffer<uint>> mSceneIB;
    std::vector<Texture> mSceneTextures;
    std::vector<StageFrameResource> mSFR;

    Scene::RenderItemsMap mRenderItems;

    std::unique_ptr<ResourceRegister> mResourceRegister;

    int mEnvMapIndex;

    uint mRTVDescriptorSize;
    uint mDSVDescriptorSize;
    uint mCBVSRVUAVDescriptorSize;
    int mCurrentFrameIndex = 0;
};