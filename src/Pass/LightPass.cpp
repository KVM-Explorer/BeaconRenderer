#include "LightPass.h"

LightPass::LightPass(ID3D12PipelineState *pso, ID3D12RootSignature *rs) :
    PassBase(pso, rs)
{
}

void LightPass::SetRenderTarget(ID3D12Resource *target, CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle)
{
    mTarget = target;
    mRtvHandle = rtvHandle;
}

void LightPass::SetGBuffer(ID3D12DescriptorHeap *srvHeap, CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle)
{
    mSrvHeap = srvHeap;
    mGBufferHandle = srvHandle;
}

void LightPass::SetTexture(ID3D12DescriptorHeap *srvHeap, CD3DX12_GPU_DESCRIPTOR_HANDLE srvHandle)
{
    mSrvHeap = srvHeap;
    mTextureHandle = srvHandle;
}

void LightPass::BeginPass(ID3D12GraphicsCommandList *cmdList)
{
    const float clearValue[] = {0, 0, 0, 1.0F};
    auto srv2rtv = CD3DX12_RESOURCE_BARRIER::Transition(mTarget,
                                                        D3D12_RESOURCE_STATE_GENERIC_READ,
                                                        D3D12_RESOURCE_STATE_RENDER_TARGET);

    cmdList->SetGraphicsRootSignature(mRS);
    cmdList->SetPipelineState(mPSO);

    cmdList->ResourceBarrier(1, &srv2rtv);
    cmdList->OMSetRenderTargets(1, &mRtvHandle, true, nullptr);
    cmdList->ClearRenderTargetView(mRtvHandle, clearValue, 0, nullptr);

    cmdList->SetDescriptorHeaps(1, &mSrvHeap);
    cmdList->SetGraphicsRootDescriptorTable(3, mGBufferHandle);
    cmdList->SetGraphicsRootDescriptorTable(4, mTextureHandle);
}

void LightPass::EndPass(ID3D12GraphicsCommandList *cmdList, D3D12_RESOURCE_STATES resultState)
{
    auto rtv2srv = CD3DX12_RESOURCE_BARRIER::Transition(mTarget,
                                                        D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                        resultState);
    cmdList->ResourceBarrier(1, &rtv2srv);
}