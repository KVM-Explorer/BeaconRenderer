#pragma once
#include "Framework/PassBase.h"

class SobelPass : public PassBase {
public:
    SobelPass(ID3D12PipelineState *pso, ID3D12RootSignature *rs);
    SobelPass(const SobelPass &) = delete;
    SobelPass &operator=(const SobelPass &) = delete;
    SobelPass(SobelPass &&) = default;
    SobelPass &operator=(SobelPass &&) = default;

    void SetTarget(ID3D12Resource *target, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle);
    void BeginPass(ID3D12GraphicsCommandList *cmdList) override;
    void ExecutePass(ID3D12GraphicsCommandList *cmdList) override;
    void EndPass(ID3D12GraphicsCommandList *cmdList, D3D12_RESOURCE_STATES resultState) override;

    void SetSrvGpuHandle(ID3D12GraphicsCommandList *cmdList, D3D12_GPU_DESCRIPTOR_HANDLE srcHandle, D3D12_GPU_DESCRIPTOR_HANDLE dstHandle);
    ID3D12Resource *Output();

private:
    ID3D12Resource *mTarget = nullptr;
    D3D12_CPU_DESCRIPTOR_HANDLE mRtvHandle;
};