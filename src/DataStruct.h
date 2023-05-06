#pragma once
#include <stdafx.h>
#include "D3DHelpler/Texture.h"
#include "D3DHelpler/UploadBuffer.h"
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

enum class EntityType {
    Sky,
    Debug,
    Opaque, // 不透明
    Test,   // Sphere Test
    Quad,   // Quad Rendering
};

enum class ShaderID {
    Sky,
    Opaque,
    OpaqueWithTexture,
    Shadow,
    Test,
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

struct MaterialInfo {
    DirectX::XMFLOAT4 BaseColor;
    DirectX::XMFLOAT3 FresnelR0;
    float Roughness;
    uint DiffuseMapIndex;
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
    uint QuadType;
    uint Padding2;
};

enum class QuadShader {
    SimpleColor,
    SimpleQuad,
    MixQuad
};

enum class Gpu {
    Integrated, // 集显
    Discrete,   // 独显
};

enum class RESOURCE {
    DISPLAY,
    BACKEND,
};

struct DescriptorMap {
    std::unordered_map<std::string, uint> CBVSRVUAV;
    std::unordered_map<std::string, uint> Sampler;
    std::unordered_map<std::string, uint> RTV;
    std::unordered_map<std::string, uint> DSV;
};

struct SceneCB{
    std::unique_ptr<UploadBuffer<SceneInfo>> SceneCB;
    std::unique_ptr<UploadBuffer<Light>> LightCB;
    std::unique_ptr<UploadBuffer<EntityInfo>> EntityCB;
    std::unique_ptr<UploadBuffer<MaterialInfo>> MaterialCB;
};
enum class Stage {
    DeferredRendering,
    CopyTexture,
    PostProcess,
};