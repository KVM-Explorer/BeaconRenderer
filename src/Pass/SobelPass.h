#pragma once
#include <stdafx.h>

class SobelPass  {
public:
    SobelPass(ID3D12PipelineState *pso, ID3D12RootSignature *rs);
    SobelPass(const SobelPass &) = delete;
    SobelPass &operator=(const SobelPass &) = delete;
    SobelPass(SobelPass &&) = default;
    SobelPass &operator=(SobelPass &&) = default;

    void SetSrvHeap(ID3D12DescriptorHeap *srvHeap);
    void SetInput(CD3DX12_GPU_DESCRIPTOR_HANDLE srcHandle);
    void SetTarget(ID3D12Resource *target, CD3DX12_GPU_DESCRIPTOR_HANDLE dstHandle);
    void BeginPass(ID3D12GraphicsCommandList *cmdList) ;
    void ExecutePass(ID3D12GraphicsCommandList *cmdList);
    void EndPass(ID3D12GraphicsCommandList *cmdList, D3D12_RESOURCE_STATES resultState) ;

private:
    ID3D12Resource *mTarget = nullptr;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mSrcHandle;
    CD3DX12_GPU_DESCRIPTOR_HANDLE mDstHandle;
    ID3D12DescriptorHeap *mSrvHeap = nullptr;
    ID3D12RootSignature* mRS;
    ID3D12PipelineState* mPSO;
};