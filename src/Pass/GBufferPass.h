#pragma once
#include <stdafx.h>

class GBufferPass {
public:
    GBufferPass(ID3D12PipelineState *pso, ID3D12RootSignature *rs);
    GBufferPass(const GBufferPass &) = delete;
    GBufferPass &operator=(const GBufferPass &) = delete;
    GBufferPass(GBufferPass &&) = default;
    GBufferPass &operator=(GBufferPass &&) = default;

    void SetRenderTarget(std::vector<ID3D12Resource *> targets, CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle);
    void SetDepthBuffer(ID3D12Resource *depthBuffer, CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle);
    void SetRTVDescriptorSize(uint size) { mRTVDescriptorSize = size; }

    void BeginPass(ID3D12GraphicsCommandList *cmdList, D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_GENERIC_READ);
    void EndPass(ID3D12GraphicsCommandList *cmdList, D3D12_RESOURCE_STATES resultState);

    static std::vector<DXGI_FORMAT> GetTargetFormat() { return mTargetFormats; }
    static uint GetTargetCount() { return mTargetCount; }
    static DXGI_FORMAT GetDepthFormat() { return mDepthFormat; }

private:
    std::vector<ID3D12Resource *> mTargets;
    ID3D12Resource *mDepthBuffer;
    static const uint mTargetCount = 4;
    inline static const std::vector<DXGI_FORMAT> mTargetFormats{
        DXGI_FORMAT_R8G8B8A8_SNORM, // NORMAL
        DXGI_FORMAT_R16G16_FLOAT,   // UV
        DXGI_FORMAT_R16_UINT,       // MaterialID
        DXGI_FORMAT_R8_UINT,        // ShaderID
    };

    static const DXGI_FORMAT mDepthFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    uint mRTVDescriptorSize;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mRtvHandle;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mDsvHandle;
    ID3D12RootSignature* mRS;
    ID3D12PipelineState* mPSO;
};