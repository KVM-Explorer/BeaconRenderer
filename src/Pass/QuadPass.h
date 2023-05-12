#pragma once
#include <stdafx.h>
#include "DataStruct.h"

class QuadPass {
public:
    QuadPass(ID3D12PipelineState *pso, ID3D12RootSignature *rs);
    QuadPass(const QuadPass &) = default;
    QuadPass &operator=(const QuadPass &) = default;
    QuadPass(QuadPass &&) = default;
    QuadPass &operator=(QuadPass &&) = default;

    void SetTarget(ID3D12Resource *target, CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle);
    void SetRenderType(QuadShader renderType);
    void SetSrvHandle(CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle);
    void BeginPass(ID3D12GraphicsCommandList *cmdList,D3D12_RESOURCE_STATES beforeState = D3D12_RESOURCE_STATE_PRESENT);
    void EndPass(ID3D12GraphicsCommandList *cmdList, D3D12_RESOURCE_STATES resultState);

private:
    QuadShader mRenderType = QuadShader::MixQuad;
    ID3D12Resource *mTarget = nullptr;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mRtvHandle;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mSrvHandle;
    ID3D12RootSignature* mRS;
    ID3D12PipelineState* mPSO;
};