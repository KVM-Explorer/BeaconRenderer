#include "Entity.h"

Entity::Entity(EntityType type) :
    mType(type)
{}

Entity &Entity::SetMaterialIndex(uint index)
{
    
}
Entity &Entity::SetEntityIndex(uint index, D3D12_GPU_VIRTUAL_ADDRESS base, uint elementSize)
{
    
}
Entity &Entity::SetVertexInfo(uint index, D3D12_GPU_VIRTUAL_ADDRESS base, uint elementSize, uint totalSize)
{
    mVertexBufferView.BufferLocation = base + index * elementSize;
    mVertexBufferView.SizeInBytes = totalSize;
    mVertexBufferView.StrideInBytes = elementSize;
    return *this;
}
Entity &Entity::SetIndexInfo(uint index, D3D12_GPU_VIRTUAL_ADDRESS base, uint elementSize, uint totalSize, bool is32Byte)
{
    mIndexBufferView.BufferLocation = base + index * elementSize;
    mIndexBufferView.SizeInBytes = totalSize;
    mIndexBufferView.Format = DXGI_FORMAT_R16_UINT;
    if (is32Byte) mIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
    return *this;
}
