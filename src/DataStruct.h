#pragma once
#include <stdafx.h>
struct SceneAdapter {
    ID3D12Device *Device;
    ID3D12GraphicsCommandList *CommandList;
    IDXGISwapChain *SwapChain;
    uint FrameWidth;
    uint FrameHeight;
    uint FrameCount;
};

struct Vertex {
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT4 Color;
};

enum class EntityType {
    Sky,
    Debug,
    Opaque, // 不透明
};



