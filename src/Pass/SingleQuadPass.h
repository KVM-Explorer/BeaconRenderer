#pragma once
#include <stdafx.h>
#include "DataStruct.h"

class SingleQuadPass {
public:
    SingleQuadPass(ID3D12PipelineState *pso, ID3D12RootSignature *rs);
    SingleQuadPass(const SingleQuadPass &) = default;
    SingleQuadPass &operator=(const SingleQuadPass &) = default;
    SingleQuadPass(SingleQuadPass &&) = default;
    SingleQuadPass &operator=(SingleQuadPass &&) = default;

    void SetSrvHeap(ID3D12DescriptorHeap *srvHeap);
    void SetTarget(ID3D12Resource *target, CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle);
    void SetSrvHandle(CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle);
    void BeginPass(ID3D12GraphicsCommandList *cmdList,D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_PRESENT) const;
    void EndPass(ID3D12GraphicsCommandList *cmdList, D3D12_RESOURCE_STATES resultState) const;

private:
    QuadShader mRenderType = QuadShader::MixQuad;
    ID3D12Resource *mTarget = nullptr;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mRtvHandle;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mSrvHandle;
    ID3D12RootSignature* mRS;
    ID3D12PipelineState *mPSO;
    ID3D12DescriptorHeap *mSrvHeap = nullptr;
};