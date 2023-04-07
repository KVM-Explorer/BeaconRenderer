#include "QuadPass.h"

QuadPass::QuadPass(ID3D12PipelineState *pso,ID3D12RootSignature *rs) :
    PassBase(pso,rs)
{
}

void QuadPass::SetTarget(ID3D12Resource *target, D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle)
{
    mTarget = target;
    mRtvHandle = rtvHandle;
}

void QuadPass::BeginPass(ID3D12GraphicsCommandList *cmdList)
{

}

void QuadPass::ExecutePass(ID3D12GraphicsCommandList *cmdList)
{
    // Draw call Quad
}

void QuadPass::EndPass(ID3D12GraphicsCommandList *cmdList, D3D12_RESOURCE_STATES resultState)
{
}