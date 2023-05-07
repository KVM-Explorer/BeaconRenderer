#include "RenderItem.h"
#include "Tools/FrameworkHelper.h"
RenderItem &RenderItem::SetVertexInfo(uint index, D3D12_GPU_VIRTUAL_ADDRESS base, uint elementSize, uint elementNum)
{
    mVertexBufferView.BufferLocation = base + index * elementSize;
    mVertexBufferView.SizeInBytes = elementSize * elementNum;
    mVertexBufferView.StrideInBytes = elementSize;
    return *this;
}
RenderItem &RenderItem::SetIndexInfo(uint index, D3D12_GPU_VIRTUAL_ADDRESS base, uint elementSize, uint elementNum, bool is32Byte)
{
    mIndexBufferView.BufferLocation = base + index * elementSize;
    mIndexBufferView.SizeInBytes = elementSize * elementNum;
    mIndexBufferView.Format = DXGI_FORMAT_R16_UINT;
    if (is32Byte) mIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
    mIndexCount = elementNum;
    return *this;
}

RenderItem &RenderItem::SetConstantInfo(uint index,
                                        uint elementSize,
                                        uint rootParameterIndex)
{
    mConstantSize = CalculateConstantBufferByteSize(elementSize);
    mConstantOffset = index;
    mRootParameterIndex = rootParameterIndex;
    return *this;
}


void RenderItem::DrawItem(ID3D12GraphicsCommandList *commandList, D3D12_GPU_VIRTUAL_ADDRESS constantAddress)
{
    // Entity Constant address
    auto address = constantAddress + mConstantOffset * mConstantSize;
    commandList->SetGraphicsRootConstantBufferView(mRootParameterIndex, address);
    commandList->IASetPrimitiveTopology(mPrimitiveType);
    commandList->IASetVertexBuffers(0, 1, &mVertexBufferView);
    commandList->IASetIndexBuffer(&mIndexBufferView);
    commandList->DrawIndexedInstanced(mIndexCount,
                                      1,
                                      0,
                                      0,
                                      0);
}
