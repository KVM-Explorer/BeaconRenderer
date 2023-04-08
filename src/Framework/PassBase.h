#pragma once
#include <stdafx.h>

class PassBase {
public:
    explicit PassBase(ID3D12PipelineState *pso, ID3D12RootSignature *rs);
    ~PassBase();
    PassBase(const PassBase &) = delete;
    PassBase &operator=(const PassBase &) = delete;
    PassBase &operator=(PassBase &&) = default;
    PassBase(PassBase &&) = default;

    virtual void BeginPass(ID3D12GraphicsCommandList *cmdList) = 0;
    virtual void EndPass(ID3D12GraphicsCommandList *cmdList, D3D12_RESOURCE_STATES resultState) = 0;
    ID3D12Resource *Output();

protected:

    ID3D12PipelineState *mPSO = nullptr;
    ID3D12RootSignature *mRS = nullptr;
};