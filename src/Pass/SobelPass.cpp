#include "SobelPass.h"
#include "Framework/GlobalResource.h"

SobelPass::SobelPass(ID3D12PipelineState *pso, ID3D12RootSignature *rs) :
    PassBase(pso, rs)
{
}

void SobelPass::SetSrvHeap(ID3D12DescriptorHeap *srvHeap)
{
    mSrvHeap = srvHeap;
}

void SobelPass::SetInput(CD3DX12_GPU_DESCRIPTOR_HANDLE srcHandle)
{
    mSrcHandle = srcHandle;
}

void SobelPass::SetTarget(ID3D12Resource *target, CD3DX12_GPU_DESCRIPTOR_HANDLE dstHandle)
{
    mTarget = target;
    mDstHandle = dstHandle;
}
void SobelPass::BeginPass(ID3D12GraphicsCommandList *cmdList)
{
    cmdList->SetComputeRootSignature(mRS);
    cmdList->SetPipelineState(mPSO);

    cmdList->SetComputeRootDescriptorTable(0, mSrcHandle);
    cmdList->SetComputeRootDescriptorTable(1, mDstHandle);
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
    // auto uav2srv = CD3DX12_RESOURCE_BARRIER::Transition(mTarget,
    //                                                     D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
    //                                                     resultState);
    // cmdList->ResourceBarrier(1, &uav2srv);
}
