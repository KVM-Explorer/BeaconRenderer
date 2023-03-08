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
                                        D3D12_GPU_VIRTUAL_ADDRESS base,
                                        uint elementSize,
                                        uint rootParameterIndex)
{
    elementSize = CalculateConstantBufferByteSize(elementSize);
    mConstantAddress = base + index * elementSize;
    mRootParameterIndex = rootParameterIndex;
    return *this;
}

void RenderItem::DrawItem(ID3D12GraphicsCommandList *commandList)
{
    commandList->SetGraphicsRootConstantBufferView(mRootParameterIndex, mConstantAddress);
    commandList->IASetPrimitiveTopology(mPrimitiveType);
    commandList->IASetVertexBuffers(0, 1, &mVertexBufferView);
    commandList->IASetIndexBuffer(&mIndexBufferView);
    commandList->DrawIndexedInstanced(mIndexCount,
                                      1,
                                      0,
                                      0,
                                      0);
}
