#pragma once
#include "Framework/PassBase.h"

class LightPass : public PassBase {
public:
    LightPass(ID3D12PipelineState *pso, ID3D12RootSignature *rs);
    LightPass(const LightPass &) = delete;
    LightPass &operator=(const LightPass &) = delete;
    LightPass(LightPass &&) = default;
    LightPass &operator=(LightPass &&) = default;

    void SetRenderTarget(ID3D12Resource *target, CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle);
    void SetGBuffer(ID3D12DescriptorHeap *srvHeap, CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle);
    void SetTexture(ID3D12DescriptorHeap *srvHeap, CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle);
    void BeginPass(ID3D12GraphicsCommandList *cmdList) override;
    void EndPass(ID3D12GraphicsCommandList *cmdList, D3D12_RESOURCE_STATES resultState) override;

private:
    ID3D12DescriptorHeap *mSrvHeap = nullptr;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mGBufferHandle;
    ID3D12Resource *mTarget = nullptr;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mRtvHandle;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mTextureHandle;
};