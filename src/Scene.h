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

/**
    @brief 场景资产管理
*/
class Scene {
public:
    explicit Scene(const std::string &root, const std::string &modelname);
    ~Scene();
    Scene(const Scene &) = delete;
    Scene(Scene &&) = default;
    Scene &operator=(const Scene &) = delete;
    Scene &operator=(Scene &&) = default;

public:
    void Init(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList);
    void RenderScene(ID3D12GraphicsCommandList *cmdList,D3D12_GPU_VIRTUAL_ADDRESS constantAddress);
    void RenderQuad(ID3D12GraphicsCommandList *cmdList,D3D12_GPU_VIRTUAL_ADDRESS constantAddress);
    void RenderSphere(ID3D12GraphicsCommandList *cmdList,D3D12_GPU_VIRTUAL_ADDRESS constantAddress);

    void UpdateCamera();
    void UpdateSceneConstant(UploadBuffer<SceneInfo> *uploader);
    void UpdateEntityConstant(UploadBuffer<EntityInfo> *uploader);
    void UpdateLightConstant(UploadBuffer<Light> *uploader);
    void UpdateMaterialConstant(UploadBuffer<MaterialInfo> *uploader);
    void UpdateMouse(float dx, float dy);

    uint GetEntityCount() const { return static_cast<uint>(mEntities.size()); };
    uint GetMaterialCount() const { return static_cast<uint>(mMaterials.size()); };

private:
    void CreateSphereTest(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList);
    void CreateQuadTest(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList);

    void LoadAssets(ID3D12Device *device, ID3D12GraphicsCommandList *commandList);
    void BuildVertex2Constant(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList);
    void CreateSceneInfo(const ModelLight &info);
    void CreateMaterials(const std::vector<ModelMaterial> &info, ID3D12Device *device, ID3D12GraphicsCommandList *commandList);
    MeshInfo AddMesh(Mesh &mesh, ID3D12Device *device, ID3D12GraphicsCommandList *commandList);
    void CreateModels(std::vector<Model> info, ID3D12Device *device, ID3D12GraphicsCommandList *commandList);

private:
    std::unordered_map<std::string, Camera> mCamera;
    std::string mRootPath;
    std::string mSceneName;

    std::unique_ptr<DataLoader> mDataLoader;

    std::vector<DirectX::XMFLOAT4X4> mTransforms;
    std::vector<Material> mMaterials;
    std::unordered_map<std::string, Mesh> mMeshesData;

    std::vector<Entity> mEntities;
    std::unordered_map<EntityType, std::vector<RenderItem>> mRenderItems;

    // Vertices & Indices
    std::vector<ModelVertex> mAllVertices;
    std::vector<uint16> mAllIndices;
    std::unique_ptr<UploadBuffer<ModelVertex>> mVerticesBuffer;
    std::unique_ptr<UploadBuffer<uint16>> mIndicesBuffer;

    // Resource
    std::vector<Texture> mTextures;
};