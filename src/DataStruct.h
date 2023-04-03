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
    DirectX::XMFLOAT3 Normal;
    DirectX::XMFLOAT2 UV;
};

struct GBufferVertex {
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT3 Normal;
};

struct ScreenQuadVertex {
    DirectX::XMFLOAT3 Position;
    DirectX::XMFLOAT2 Texcoord;
};

enum class EntityType {
    Sky,
    Debug,
    Opaque, // 不透明
    Test,   // Sphere Test
};

enum class ShaderID {
    None,
    Opaque,
    Sky,
    Shadow
};

struct MeshInfo {
    uint VertexOffset;
    uint VertexCount;
    uint IndexOffset;
    uint IndexCount;
};

struct Material {
    uint Index;
    uint DiffuseMapIndex;
    float Shineness; // radius not 0-1
    uint Padding0;
    DirectX::XMFLOAT4 BaseColor;
    std::unique_ptr<Texture> Texture;
};

struct Light {
    DirectX::XMFLOAT3 Position;
    float LightBegin;
    DirectX::XMFLOAT3 LightStrengh;
    float LightEnd;
};
struct SceneInfo {
    DirectX::XMFLOAT4X4 View;
    DirectX::XMFLOAT4X4 InvView;
    DirectX::XMFLOAT4X4 Project;
    DirectX::XMFLOAT4X4 InvProject;
    DirectX::XMFLOAT4X4 ViewProject;
    DirectX::XMFLOAT4X4 InvViewProject;
    DirectX::XMFLOAT4X4 InvScreenModel;
    DirectX::XMFLOAT3 EyePosition;
    float Padding0;
    DirectX::XMFLOAT4 Ambient;
};

struct Mesh {
    std::vector<ModelVertex> Vertices;
    std::vector<uint16> Indices;
};
struct EntityInfo {
    DirectX::XMFLOAT4X4 Transform;
    uint MaterialIndex;
    uint ShaderID;
    uint Padding1;
    uint Padding2;
};

enum class QuadShader {
    SimpleColor,
    SimpleQuad,
    MixQuad
};