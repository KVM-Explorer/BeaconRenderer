#pragma once
#include <stdafx.h>
class LightPass {
public:
    LightPass(ID3D12PipelineState *pso, ID3D12RootSignature *rs);
    LightPass(const LightPass &) = default;
    LightPass &operator=(const LightPass &) = default;
    LightPass(LightPass &&) = default;
    LightPass &operator=(LightPass &&) = default;

    void SetRenderTarget(ID3D12Resource *target, CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle);
    void SetGBuffer(ID3D12DescriptorHeap *srvHeap, CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle);
    void SetTexture(ID3D12DescriptorHeap *srvHeap, CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle);
    void SetCubeMap(ID3D12DescriptorHeap *srvHeap, CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle);
    void BeginPass(ID3D12GraphicsCommandList *cmdList, D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_GENERIC_READ) const;
    void EndPass(ID3D12GraphicsCommandList *cmdList, D3D12_RESOURCE_STATES resultState) const;

private:
    ID3D12DescriptorHeap *mSrvHeap = nullptr;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mGBufferHandle;
    ID3D12Resource *mTarget = nullptr;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mRtvHandle;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mTextureHandle;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mCubeMapHandle;
    ID3D12RootSignature *mRS;
    ID3D12PipelineState *mPSO;
};