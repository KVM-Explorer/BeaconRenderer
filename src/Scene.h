#pragma once
#include <stdafx.h>
#include <unordered_map>
#include "Tools/Camera.h"
#include "D3DHelpler/DescroptorHeap.h"
#include "D3DHelpler/DefaultBuffer.h"
#include "DataStruct.h"
#include "Tools/DataLoader.h"
#include "D3DHelpler/Entity.h"
#include "D3DHelpler/UploadBuffer.h"
#include "D3DHelpler/RenderItem.h"

class Scene {
public:
    explicit Scene(const std::wstring &root, const std::wstring &modelname);
    Scene(const Scene &) = delete;
    Scene(Scene &&) = default;
    Scene &operator=(const Scene &) = delete;
    Scene &operator=(Scene &&) = default;

public:
    void Init(SceneAdapter &adapter);
    void RenderUI();
    void RenderScene(ID3D12GraphicsCommandList *commandList, uint frameIndex);
    void UpdateScene();

private:
    void CreateRTV(ID3D12Device *device, IDXGISwapChain *swapChain, uint frameCount);
    void CreatePipelineStateObject(ID3D12Device *device);
    void CreateRootSignature(ID3D12Device *device);
    void CreateInputLayout();
    void CreateTriangleVertex(ID3D12Device *device, ID3D12GraphicsCommandList *commandList);
    void CompileShaders();
    std::array<CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();
    void CreateCommonConstant();

    void LoadAssets(ID3D12Device *device, ID3D12GraphicsCommandList *commandList);
    void CreateSceneInfo(const ModelLight &info);
    void CreateMaterials(const std::vector<ModelMaterial> &info, ID3D12Device *device, ID3D12GraphicsCommandList *commandList);
    MeshInfo CreateMeshes(Mesh &mesh, ID3D12Device *device, ID3D12GraphicsCommandList *commandList);
    void CreateModels(std::vector<Model> info, ID3D12Device *device, ID3D12GraphicsCommandList *commandList);

    void RenderTriangleScene(ID3D12GraphicsCommandList *commandList, uint frameIndex);
    void RenderModelScene(ID3D12GraphicsCommandList *commandList, uint frameIndex);

private:
    uint mEntityCount = 0;
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSO;
    std::unordered_map<std::string, ComPtr<ID3D12RootSignature>> mSignature;
    std::unordered_map<std::string, ComPtr<ID3DBlob>> mShaders;
    std::unordered_map<std::string, Camera> mCamera;
    std::wstring mRootPath;
    std::wstring mSceneName;

    std::unique_ptr<DataLoader> mDataLoader;

    std::unique_ptr<DescriptorHeap> RTVDescriptorHeap;
    std::vector<ComPtr<ID3D12Resource>> RTVBuffer;
    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;

    std::unique_ptr<DefaultBuffer> mVertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;

    std::vector<DirectX::XMFLOAT4X4> mTransforms; // object constant
    SceneInfo mSceneInfo;                         // common constant
    std::vector<Material> mMaterials;
    std::unordered_map<std::string, Mesh> mMeshesData;

    std::unique_ptr<UploadBuffer<EntityInfo>> mObjectConstant;

    std::vector<Entity> mEntities;
    std::unordered_map<EntityType, std::vector<RenderItem>> mRenderItems;

    std::vector<ModelVertex> mAllVertices;
    std::vector<uint16> mAllIndices;
    std::unique_ptr<UploadBuffer<ModelVertex>> VerticesBuffer;
    std::unique_ptr<UploadBuffer<uint16>> IndicesBuffer;
};