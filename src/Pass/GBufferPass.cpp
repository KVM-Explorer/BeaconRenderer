#include "GBufferPass.h"
#include "Framework/GlobalResource.h"

GBufferPass::GBufferPass(ID3D12PipelineState *pso, ID3D12RootSignature *rs) :
    PassBase(pso, rs)
{
}

void GBufferPass::SetTarget(std::array<ID3D12Resource *, 3> targets, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle, D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle)
{
    mTargets = targets;
    mRtvHandle = rtvHandle;
    mDsvHandle = dsvHandle;
}
void GBufferPass::BeginPass(ID3D12GraphicsCommandList *cmdList)
{
    float clearColor[4] = {0, 0, 0, 1.0F};

    for (uint i = 0; i < mTargets.size(); i++) {
        // 1. rtvHandle
        // 2. Srv -> Rtv Barrier
        // 3. Clear Value
    }

    cmdList->OMSetRenderTargets(3, &mRtvHandle, true, &mDsvHandle);
    // cmdList->SetDescriptorHeaps(srvHeaps.size(), srvHeaps.data());
    // cmdList->SetGraphicsRootSignature(mRS);
    // cmdList->SetGraphicsRootDescriptorTable(4, GResource::TextureManager->GetTexture2DDescriptoHeap()->GPUHandle(mSrvIndexBase));
}

void GBufferPass::ExecutePass(ID3D12GraphicsCommandList *cmdList)
{
}

void GBufferPass::EndPass(ID3D12GraphicsCommandList *cmdList, D3D12_RESOURCE_STATES resultState)
{
    //  GBuffer + Texture -> SRV
    // for (uint i = 0; i < mTargets.size(); i++) {
    //     auto rtvTexture = GResource::TextureManager->GetTexture(mGbufferTextureIndex.at(i));
    //     auto rtv2srvBarrier = CD3DX12_RESOURCE_BARRIER::Transition(rtvTexture->Resource(),
    //                                                                D3D12_RESOURCE_STATE_RENDER_TARGET,
    //                                                                D3D12_RESOURCE_STATE_GENERIC_READ);
    //     cmdList->ResourceBarrier(1, &rtv2srvBarrier);
    // }
    // auto depthTexture = GResource::TextureManager->GetTexture(mDepthTextureIndex);
    // auto depth2srvBarrier = CD3DX12_RESOURCE_BARRIER::Transition(depthTexture->Resource(),
    //                                                              D3D12_RESOURCE_STATE_DEPTH_WRITE,
    //                                                              D3D12_RESOURCE_STATE_GENERIC_READ);
    // cmdList->ResourceBarrier(1, &depth2srvBarrier);
}
