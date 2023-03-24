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

    void Init(ID3D12Device *device);

    // GBuffer & Quad
    void CreatePSOs(ID3D12Device *device,
                    std::vector<D3D12_INPUT_ELEMENT_DESC> gBufferInputLayout,
                    ComPtr<ID3DBlob> gBufferVS,
                    ComPtr<ID3DBlob> gBufferPS,
                    std::vector<D3D12_INPUT_ELEMENT_DESC> lightPassInputLayout,
                    ComPtr<ID3DBlob> QuadRenderingVS,
                    ComPtr<ID3DBlob> QuadRenderingPS);

    void GBufferPass(ID3D12GraphicsCommandList *cmdList);
    void LightingPass(ID3D12GraphicsCommandList *cmdList);

private:
    void CreateRTV(ID3D12Device *device);
    void CreateDSV(ID3D12Device *device);
    void CreateRootSignature(ID3D12Device *device);

    ComPtr<ID3D12PipelineState> mGBufferPSO;
    ComPtr<ID3D12PipelineState> mLightingPSO;

    ComPtr<ID3D12RootSignature> mRootSignature;

    DXGI_FORMAT mDSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    const static int mRTNum = 3;
    std::array<DXGI_FORMAT, mRTNum> mRTVFormat =
        {DXGI_FORMAT_R11G11B10_FLOAT,
         DXGI_FORMAT_R8G8B8A8_SNORM,
         DXGI_FORMAT_R8G8B8A8_UNORM};

    std::unique_ptr<DescriptorHeap> mRTVDescriptorHeap;
    std::unique_ptr<DescriptorHeap> mDSVDescriptorHeap;

    int mDepthTextureIndex = -1;
    std::array<int, 3> mGbufferTextureIndex{-1, -1, -1};

    uint mScreenWidth;
    uint mScreenHeight;
};