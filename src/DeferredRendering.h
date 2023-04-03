#pragma once
#include <stdafx.h>
#include <tuple>
#include "D3DHelpler/DescroptorHeap.h"
#include "Framework/GlobalResource.h"

class DeferredRendering {
public:
    DeferredRendering(const DeferredRendering &) = delete;
    DeferredRendering &operator=(const DeferredRendering &) = delete;
    DeferredRendering(DeferredRendering &&) = default;
    DeferredRendering &operator=(DeferredRendering &&) = default;

    DeferredRendering(uint width, uint height);
    ~DeferredRendering();
    void Init(ID3D12Device *device);

    // GBuffer & Quad

    void GBufferPass(ID3D12GraphicsCommandList *cmdList);
    void LightPass(ID3D12GraphicsCommandList *cmdList);
    uint GetDepthTexture() const
    {
        return mDepthTextureIndex;
    }

private:
    void CreateInputLayout();
    void CreateRTV(ID3D12Device *device);
    void CreateDSV(ID3D12Device *device);
    void CompileShaders();
    void CreateRootSignature(ID3D12Device *device);
    void CreatePSOs(ID3D12Device *device);

    ComPtr<ID3D12PipelineState> mGBufferPSO;
    ComPtr<ID3D12PipelineState> mLightingPSO;

    ComPtr<ID3D12RootSignature> mRootSignature;

    DXGI_FORMAT mDSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    const static int mRTNum = 4;
    std::array<DXGI_FORMAT, mRTNum> mRTVFormat =
        {
            DXGI_FORMAT_R8G8B8A8_SNORM, // NORMAL
            DXGI_FORMAT_R16G16_FLOAT,   // UV
            DXGI_FORMAT_R16_UINT,       // MaterialID
            DXGI_FORMAT_R8_UINT         // ShaderID
    };

    std::unique_ptr<DescriptorHeap> mRTVDescriptorHeap;
    std::unique_ptr<DescriptorHeap> mDSVDescriptorHeap;

    int mDepthTextureIndex = -1;
    uint mSrvIndexBase = 0;
    std::array<int, mRTNum> mGbufferTextureIndex;

    uint mScreenWidth;
    uint mScreenHeight;
    std::unordered_map<std::string, std::vector<D3D12_INPUT_ELEMENT_DESC>> mInputLayout;
    std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
};