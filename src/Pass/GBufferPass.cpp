#include "GBufferPass.h"
#include "Framework/GlobalResource.h"

GBufferPass::GBufferPass(ID3D12PipelineState *pso, ID3D12RootSignature *rs) :
    PassBase(pso, rs)
{
}

void GBufferPass::SetRenderTarget(std::vector<ID3D12Resource *> targets, CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle)
{
    mTargets = targets;
    mRtvHandle = rtvHandle;
}
void GBufferPass::SetDepthBuffer(ID3D12Resource *depthBuffer, CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle)
{
    mDepthBuffer = depthBuffer;
    mDsvHandle = dsvHandle;
}

void GBufferPass::BeginPass(ID3D12GraphicsCommandList *cmdList)
{
    cmdList->SetGraphicsRootSignature(mRS);
    cmdList->SetPipelineState(mPSO);
    cmdList->OMSetRenderTargets(4, &mRtvHandle, true, &mDsvHandle);

    float clearColor[4] = {0, 0, 0, 1.0F};

    CD3DX12_CPU_DESCRIPTOR_HANDLE targetRtvHandle = mRtvHandle;
    for (uint i = 0; i < mTargets.size(); i++) {
        auto srv2rtv = CD3DX12_RESOURCE_BARRIER::Transition(mTargets.at(i),
                                                            D3D12_RESOURCE_STATE_GENERIC_READ,
                                                            D3D12_RESOURCE_STATE_RENDER_TARGET);
        cmdList->ResourceBarrier(1, &srv2rtv);
        cmdList->ClearRenderTargetView(targetRtvHandle, clearColor, 0, nullptr);
        targetRtvHandle.Offset(1,mRTVDescriptorSize);
    }

    auto srv2dsv = CD3DX12_RESOURCE_BARRIER::Transition(mDepthBuffer,
                                                        D3D12_RESOURCE_STATE_GENERIC_READ,
                                                        D3D12_RESOURCE_STATE_DEPTH_WRITE);
    cmdList->ResourceBarrier(1, &srv2dsv);
    cmdList->ClearDepthStencilView(mDsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0F, 0, 0, nullptr);
}

void GBufferPass::EndPass(ID3D12GraphicsCommandList *cmdList, D3D12_RESOURCE_STATES resultState)
{
    //  GBuffer + Depth -> SRV
    std::array<D3D12_RESOURCE_BARRIER,mTargetCount + 1> rtv2srvs;
    for (uint i = 0; i < mTargets.size(); i++) {
        auto rtv2srv = CD3DX12_RESOURCE_BARRIER::Transition(mTargets.at(i),
                                                            D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                            D3D12_RESOURCE_STATE_GENERIC_READ);
        rtv2srvs.at(i) = rtv2srv;
    }

    auto dsv2srv = CD3DX12_RESOURCE_BARRIER::Transition(mDepthBuffer,
                                                        D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                                        D3D12_RESOURCE_STATE_GENERIC_READ);
    rtv2srvs.at(mTargetCount) = dsv2srv;
    cmdList->ResourceBarrier(rtv2srvs.size(), rtv2srvs.data());
}
