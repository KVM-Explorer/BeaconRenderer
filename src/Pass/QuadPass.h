#pragma once
#include "Framework/PassBase.h"

class QuadPass : public PassBase {
public:
    QuadPass(ID3D12PipelineState *pso, ID3D12RootSignature *rs);
    QuadPass(const QuadPass &) = delete;
    QuadPass &operator=(const QuadPass &) = delete;
    QuadPass(QuadPass &&) = default;
    QuadPass &operator=(QuadPass &&) = default;

    void SetTarget(ID3D12Resource *target, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle);
    void BeginPass(ID3D12GraphicsCommandList *cmdList) override;
    void ExecutePass(ID3D12GraphicsCommandList *cmdList) override;
    void EndPass(ID3D12GraphicsCommandList *cmdList, D3D12_RESOURCE_STATES resultState) override;
    ID3D12Resource *Output();

private:
    ID3D12Resource *mTarget = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE mRtvHandle;
};