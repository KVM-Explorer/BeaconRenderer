#pragma once
#include <stdafx.h>
#include "DataStruct.h"

class MixQuadPass {
public:
    MixQuadPass(ID3D12PipelineState *pso, ID3D12RootSignature *rs);
    MixQuadPass(const MixQuadPass &) = default;
    MixQuadPass &operator=(const MixQuadPass &) = default;
    MixQuadPass(MixQuadPass &&) = default;
    MixQuadPass &operator=(MixQuadPass &&) = default;

    void SetTarget(ID3D12Resource *target, CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle);
    void SetRenderType(QuadShader renderType);
    void SetSrvHandle(CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle);
    void BeginPass(ID3D12GraphicsCommandList *cmdList,D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_PRESENT,bool isTargetSwapChain = true) const;
    void EndPass(ID3D12GraphicsCommandList *cmdList, D3D12_RESOURCE_STATES resultState) const;

private:
    QuadShader mRenderType = QuadShader::MixQuad;
    ID3D12Resource *mTarget = nullptr;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mRtvHandle;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mSrvHandle;
    ID3D12RootSignature* mRS;
    ID3D12PipelineState* mPSO;
};