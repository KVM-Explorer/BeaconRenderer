#include "QuadPass.h"

QuadPass::QuadPass(ID3D12PipelineState *pso, ID3D12RootSignature *rs) :
    PassBase(pso, rs)
{
}

void QuadPass::SetTarget(ID3D12Resource *target, CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle)
{
    mTarget = target;
    mRtvHandle = rtvHandle;
}

void QuadPass::SetRenderType(QuadShader renderType)
{
    mRenderType = renderType;
}

void QuadPass::SetSrvHandle(CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle)
{
    mSrvHandle = srvHandle;
}

void QuadPass::BeginPass(ID3D12GraphicsCommandList *cmdList)
{
    auto present2rtv = CD3DX12_RESOURCE_BARRIER::Transition(mTarget,
                                                            D3D12_RESOURCE_STATE_PRESENT,
                                                            D3D12_RESOURCE_STATE_RENDER_TARGET);
    uint renderType = static_cast<uint>(mRenderType);

    cmdList->SetGraphicsRootSignature(mRS);
    cmdList->SetPipelineState(mPSO);
    cmdList->ResourceBarrier(1, &present2rtv);
    cmdList->OMSetRenderTargets(1, &mRtvHandle, true, nullptr);
    cmdList->ClearRenderTargetView(mRtvHandle, DirectX::Colors::LightSteelBlue, 0, nullptr);

    cmdList->SetGraphicsRootDescriptorTable(4, mSrvHandle);
}

void QuadPass::EndPass(ID3D12GraphicsCommandList *cmdList, D3D12_RESOURCE_STATES resultState)
{
    // auto rtv2state = CD3DX12_RESOURCE_BARRIER::Transition(mTarget,
    //                                                       D3D12_RESOURCE_STATE_RENDER_TARGET,
    //                                                       resultState);
    // cmdList->ResourceBarrier(1, &rtv2state);
}