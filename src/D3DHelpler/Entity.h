
#pragma once
#include "DataStruct.h"

class Entity {
public:
    Entity(EntityType type);
    Entity &SetVertexInfo(uint index, D3D12_GPU_VIRTUAL_ADDRESS base, uint elementSize, uint totalSize);
    Entity &SetIndexInfo(uint index, D3D12_GPU_VIRTUAL_ADDRESS base, uint elementSize, uint totalSize, bool is32Byte = false);

public:
    DirectX::XMMATRIX Transform = DirectX::XMMatrixIdentity();
    uint MaterialIndex;
    uint EntityIndex;

private:
    D3D12_GPU_VIRTUAL_ADDRESS mConstantAddress;
    D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
    D3D12_INDEX_BUFFER_VIEW mIndexBufferView;
    D3D_PRIMITIVE_TOPOLOGY mPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    EntityType mType;
};