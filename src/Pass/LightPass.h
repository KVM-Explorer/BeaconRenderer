#pragma once
#include "Framework/PassBase.h"

class LightPass : public PassBase {
public:
    LightPass(ID3D12PipelineState *pso, ID3D12RootSignature *rs);
    LightPass(const LightPass &) = delete;
    LightPass &operator=(const LightPass &) = delete;
    LightPass(LightPass &&) = default;
    LightPass &operator=(LightPass &&) = default;

    void SetTarget(ID3D12Resource *target, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle);
    void BeginPass(ID3D12GraphicsCommandList *cmdList) override;
    void ExecutePass(ID3D12GraphicsCommandList *cmdList) override;
    void EndPass(ID3D12GraphicsCommandList *cmdList, D3D12_RESOURCE_STATES resultState) override;
    ID3D12Resource *Output();

private:
    ID3D12Resource *mTarget = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE mRtvHandle;
};