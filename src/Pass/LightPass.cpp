#include "LightPass.h"

LightPass::LightPass(ID3D12PipelineState *pso,ID3D12RootSignature *rs) :
    PassBase(pso,rs)
{
}

void LightPass::SetTarget(ID3D12Resource *target, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle)
{
    mTarget = target;
    mRtvHandle = rtvHandle;
}

void LightPass::BeginPass(ID3D12GraphicsCommandList *cmdList)
{
    const float clearValue[] = {0, 0, 0, 1.0F};

    cmdList->SetPipelineState(mPSO);
    cmdList->OMSetRenderTargets(1, &mRtvHandle, true, nullptr);
    cmdList->ClearRenderTargetView(mRtvHandle, clearValue, 0, nullptr);
    auto srv2rtv = CD3DX12_RESOURCE_BARRIER::Transition(mTarget,
                                                        D3D12_RESOURCE_STATE_GENERIC_READ,
                                                        D3D12_RESOURCE_STATE_RENDER_TARGET);
    cmdList->ResourceBarrier(1, &srv2rtv);
}

void LightPass::ExecutePass(ID3D12GraphicsCommandList *cmdList)
{
    // Draw call Quad
}

void LightPass::EndPass(ID3D12GraphicsCommandList *cmdList, D3D12_RESOURCE_STATES resultState)
{
    auto rtv2srv = CD3DX12_RESOURCE_BARRIER::Transition(mTarget,
                                                        D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                        resultState);
    cmdList->ResourceBarrier(1, &rtv2srv);
}