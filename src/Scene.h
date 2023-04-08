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
#include "DeferredRendering.h"

/**
    @brief 场景资产管理
*/
class Scene {
public:
    explicit Scene(const std::string &root, const std::string &modelname);
    Scene(const Scene &) = delete;
    Scene(Scene &&) = default;
    Scene &operator=(const Scene &) = delete;
    Scene &operator=(Scene &&) = default;

public:
    void Init(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList);
    void RenderScene(ID3D12GraphicsCommandList *cmdList);
    void RenderQuad(ID3D12GraphicsCommandList *cmdList);
    void RenderSphere(ID3D12GraphicsCommandList *cmdList);
    void UpdateScene();
    void UpdateMouse(float dx, float dy);

private:
    void CreateRTV(ID3D12Device *device, IDXGISwapChain *swapChain, uint frameCount);

    std::array<CD3DX12_STATIC_SAMPLER_DESC, 7> GetStaticSamplers();
    void CreateCommonConstant(ID3D12Device *device);
    void CreateDescriptorHeaps2Descriptors(ID3D12Device *device, uint width, uint height);

    void CreateSphereTest(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList);
    void CreateQuadTest(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList);

    void LoadAssets(ID3D12Device *device, ID3D12GraphicsCommandList *commandList);
    void BuildVertex2Constant(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList);
    void CreateSceneInfo(const ModelLight &info);
    void CreateMaterials(const std::vector<ModelMaterial> &info, ID3D12Device *device, ID3D12GraphicsCommandList *commandList);
    MeshInfo AddMesh(Mesh &mesh, ID3D12Device *device, ID3D12GraphicsCommandList *commandList);
    void CreateModels(std::vector<Model> info, ID3D12Device *device, ID3D12GraphicsCommandList *commandList);

    void UpdateSceneConstant();
    void UpdateEntityConstant();
    void UpdateCamera();
    void UpdateLight();
    void UpdateMaterial();

private:
    std::unordered_map<std::string, Camera> mCamera;
    std::string mRootPath;
    std::string mSceneName;

    std::unique_ptr<DataLoader> mDataLoader;

    std::vector<DirectX::XMFLOAT4X4> mTransforms;
    std::vector<Material> mMaterials;
    std::unordered_map<std::string, Mesh> mMeshesData;

    std::unique_ptr<UploadBuffer<EntityInfo>> mObjectConstant;
    std::unique_ptr<UploadBuffer<SceneInfo>> mSceneConstant;
    std::unique_ptr<UploadBuffer<Light>> mLightConstant;
    std::unique_ptr<UploadBuffer<MaterialInfo>> mMaterialSR;
    std::vector<Entity> mEntities;
    std::unordered_map<EntityType, std::vector<RenderItem>> mRenderItems;

    // Vertices & Indices
    std::vector<ModelVertex> mAllVertices;
    std::vector<uint16> mAllIndices;
    std::unique_ptr<UploadBuffer<ModelVertex>> mVerticesBuffer;
    std::unique_ptr<UploadBuffer<uint16>> mIndicesBuffer;

    std::unique_ptr<DescriptorHeap> mRTVDescriptorHeap;
    std::unique_ptr<DescriptorHeap> mSRVDescriptorHeap;
};