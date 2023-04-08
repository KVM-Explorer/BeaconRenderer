#pragma once
#include "Framework/PassBase.h"
#include "DataStruct.h"

class QuadPass : public PassBase {
public:
    QuadPass(ID3D12PipelineState *pso, ID3D12RootSignature *rs);
    QuadPass(const QuadPass &) = delete;
    QuadPass &operator=(const QuadPass &) = delete;
    QuadPass(QuadPass &&) = default;
    QuadPass &operator=(QuadPass &&) = default;

    void SetTarget(ID3D12Resource *target, CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle);
    void SetRenderType(QuadShader renderType);
    void SetSrvHandle(CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle);
    void BeginPass(ID3D12GraphicsCommandList *cmdList) override;
    void EndPass(ID3D12GraphicsCommandList *cmdList, D3D12_RESOURCE_STATES resultState) override;

private:
    QuadShader mRenderType = QuadShader::MixQuad;
    ID3D12Resource *mTarget = nullptr;
    CD3DX12_CPU_DESCRIPTOR_HANDLE mRtvHandle;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mSrvHandle;
};