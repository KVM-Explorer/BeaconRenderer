#pragma once
#include "Framework/PassBase.h"

class GBufferPass : public PassBase {
public:
    GBufferPass(ID3D12PipelineState *pso, ID3D12RootSignature *rs);
    GBufferPass(const GBufferPass &) = delete;
    GBufferPass &operator=(const GBufferPass &) = delete;
    GBufferPass(GBufferPass &&) = default;
    GBufferPass &operator=(GBufferPass &&) = default;

    void SetTarget(std::array<ID3D12Resource *, 3> targets, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle);
    void BeginPass(ID3D12GraphicsCommandList *cmdList) override;
    void ExecutePass(ID3D12GraphicsCommandList *cmdList) override;
    void EndPass(ID3D12GraphicsCommandList *cmdList, D3D12_RESOURCE_STATES resultState) override;

private:
    std::array<ID3D12Resource *, 3> mTargets;
    D3D12_CPU_DESCRIPTOR_HANDLE mRtvHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE mDsvHandle;
};