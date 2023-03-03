#pragma once
#include <stdafx.h>
#include "D3DHelpler/Texture.h"
struct SceneAdapter {
    ID3D12Device *Device;
    ID3D12GraphicsCommandList *CommandList;
    IDXGISwapChain *SwapChain;
    uint FrameWidth;
    uint FrameHeight;
    uint FrameCount;
};

// Test Triangle
struct Vertex {
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT4 Color;
};

// import model
struct ModelVertex {
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT2 UV;
    DirectX::XMFLOAT3 Normal;
};

enum class EntityType {
    Sky,
    Debug,
    Opaque, // 不透明
};


struct Material {
    uint Index;
    uint DiffuseMapIndex;
    float Shineness; // radius not 0-1
    uint Padding0;
    DirectX::XMFLOAT4 BaseColor;
    std::unique_ptr<Texture> Texture;
};
struct SceneInfo {

};

struct Mesh {
    std::vector<ModelVertex> Vertices;
    std::vector<uint16> Indices;
};