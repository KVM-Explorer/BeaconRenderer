#pragma once
#include <stdafx.h>

class RenderItem {
public:
    RenderItem &SetVertexInfo(uint index, D3D12_GPU_VIRTUAL_ADDRESS base, uint elementSize, uint elementNum);
    RenderItem &SetIndexInfo(uint index, D3D12_GPU_VIRTUAL_ADDRESS base, uint elementSize, uint elementNum, bool is32Byte = false);
    RenderItem &SetConstantInfo(uint index, uint elementSize, uint rootParameterIndex);

    void DrawItem(ID3D12GraphicsCommandList *commandList,D3D12_GPU_VIRTUAL_ADDRESS constantAddress);

private:
    uint mIndexCount;
    uint mRootParameterIndex;
    uint mConstantSize;
    uint mConstantOffset;
    D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
    D3D12_INDEX_BUFFER_VIEW mIndexBufferView;
    D3D_PRIMITIVE_TOPOLOGY mPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
};
