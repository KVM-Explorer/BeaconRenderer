#include "SobelPass.h"
#include "Framework/GlobalResource.h"

SobelPass::SobelPass(ID3D12PipelineState *pso, ID3D12RootSignature *rs) :
    PassBase(pso, rs)
{
}

void SobelPass::SetTarget(ID3D12Resource *target, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle)
{
    mTarget = target;
    mRtvHandle = rtvHandle;
}

void SobelPass::BeginPass(ID3D12GraphicsCommandList *cmdList)
{
    cmdList->SetComputeRootSignature(mRS);
    cmdList->SetPipelineState(mPSO);
}

void SobelPass::ExecutePass(ID3D12GraphicsCommandList *cmdList)
{
    const uint threadNum = 16;
    uint numGroupX = GResource::Width / threadNum;
    uint numGroupY = GResource::Height / threadNum;
    cmdList->Dispatch(numGroupX, numGroupY, 1);
}

void SobelPass::EndPass(ID3D12GraphicsCommandList *cmdList, D3D12_RESOURCE_STATES resultState)
{
    auto rtv2srv = CD3DX12_RESOURCE_BARRIER::Transition(mTarget,
                                                        D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                        resultState);
    cmdList->ResourceBarrier(1, &rtv2srv);
}

void SobelPass::SetSrvGpuHandle(ID3D12GraphicsCommandList *cmdList, D3D12_GPU_DESCRIPTOR_HANDLE srcHandle, D3D12_GPU_DESCRIPTOR_HANDLE dstHandle)
{
    cmdList->SetComputeRootDescriptorTable(0, srcHandle);
    cmdList->SetComputeRootDescriptorTable(1, dstHandle);
}
