#include "SingleQuadPass.h"

SingleQuadPass::SingleQuadPass(ID3D12PipelineState *pso, ID3D12RootSignature *rs) :
    mPSO(pso),mRS(rs)
{
}

void SingleQuadPass::SetTarget(ID3D12Resource *target, CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle)
{
    mTarget = target;
    mRtvHandle = rtvHandle;
}

void SingleQuadPass::SetSrvHandle(CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle)
{
    mSrvHandle = srvHandle;
}

void SingleQuadPass::BeginPass(ID3D12GraphicsCommandList *cmdList, D3D12_RESOURCE_STATES beforeState) const
{
    auto present2rtv = CD3DX12_RESOURCE_BARRIER::Transition(mTarget,
                                                            beforeState,
                                                            D3D12_RESOURCE_STATE_RENDER_TARGET);
    uint renderType = static_cast<uint>(mRenderType);

    cmdList->SetGraphicsRootSignature(mRS);
    cmdList->SetPipelineState(mPSO);
    cmdList->ResourceBarrier(1, &present2rtv);
    cmdList->OMSetRenderTargets(1, &mRtvHandle, true, nullptr);
    cmdList->ClearRenderTargetView(mRtvHandle, DirectX::Colors::LightSteelBlue, 0, nullptr);

    cmdList->SetGraphicsRootDescriptorTable(3, mSrvHandle);// TODO update src Index
}

void SingleQuadPass::EndPass(ID3D12GraphicsCommandList *cmdList, D3D12_RESOURCE_STATES resultState) const
{
    auto rtv2state = CD3DX12_RESOURCE_BARRIER::Transition(mTarget,
                                                          D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                          resultState);
    cmdList->ResourceBarrier(1, &rtv2state);
}