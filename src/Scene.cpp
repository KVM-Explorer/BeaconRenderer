#include "Scene.h"
#include "Tools/DataLoader.h"
#include "Tools/FrameworkHelper.h"
#include "Framework/Application.h"
#include "Framework/GlobalResource.h"
#include <GeometryGenerator.h>

Scene::Scene(const std::string &root, const std::string &modelname) :
    mRootPath(root),
    mSceneName(modelname)
{
}

Scene::~Scene()
{
    mVerticesBuffer.reset();
    mIndicesBuffer.reset();
    mTextures.clear();
}

void Scene::Init(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList, DescriptorHeap *descriptorHeap)
{
    CreateSphereTest(device, cmdList);
    CreateQuadTest(device, cmdList);
    auto dataLoader = std::make_unique<DataLoader>(mRootPath, mSceneName);
    LoadAssets(device, cmdList, descriptorHeap, dataLoader.get(), mTextures);
    mVerticesBuffer = std::make_unique<UploadBuffer<ModelVertex>>(device, mAllVertices.size(), false);
    mIndicesBuffer = std::make_unique<UploadBuffer<UINT16>>(device, mAllIndices.size(), false);
    BuildVertex2Constant(device, cmdList, mVerticesBuffer.get(), mIndicesBuffer.get());

    mCamera["default"].SetPosition(0, 0, -10.5F);
    mCamera["default"].SetLens(0.25f * MathHelper::Pi, GResource::Width / GResource::Height, 1, 1000);
}

void Scene::InitWithBackend(ID3D12Device *device,
                            ID3D12GraphicsCommandList *cmdList,
                            DescriptorHeap *srvDescriptorHeap,
                            std::unique_ptr<UploadBuffer<ModelVertex>> &verticesBuffer,
                            std::unique_ptr<UploadBuffer<uint16>> &indicesBuffer,
                            std::vector<Texture> &textures)
{
    Reset();
    CreateSphereTest(device, cmdList);
    CreateQuadTest(device, cmdList);
    auto dataLoader = std::make_unique<DataLoader>(mRootPath, mSceneName);
    LoadAssets(device, cmdList, srvDescriptorHeap, dataLoader.get(), textures);
    verticesBuffer = std::make_unique<UploadBuffer<ModelVertex>>(device, mAllVertices.size(), false);
    indicesBuffer = std::make_unique<UploadBuffer<UINT16>>(device, mAllIndices.size(), false);
    BuildVertex2Constant(device, cmdList, verticesBuffer.get(), indicesBuffer.get());

    mCamera["default"].SetPosition(0, 0, -10.5F);
    mCamera["default"].SetLens(0.25f * MathHelper::Pi, GResource::Width / GResource::Height, 1, 1000);
}

void Scene::InitWithDisplay(ID3D12Device *device,
                            ID3D12GraphicsCommandList *cmdList,
                            std::unique_ptr<UploadBuffer<ModelVertex>> &verticesBuffer,
                            std::unique_ptr<UploadBuffer<UINT16>> &indicesBuffer)
{
    Reset();
    CreateQuadTest(device, cmdList);
    verticesBuffer = std::make_unique<UploadBuffer<ModelVertex>>(device, mAllVertices.size(), false);
    indicesBuffer = std::make_unique<UploadBuffer<UINT16>>(device, mAllIndices.size(), false);
    BuildVertex2Constant(device, cmdList, verticesBuffer.get(), indicesBuffer.get());
}

void Scene::RenderScene(ID3D12GraphicsCommandList *cmdList, D3D12_GPU_VIRTUAL_ADDRESS constantAddress, RenderItemsMap *renderItems)
{
    if (renderItems == nullptr) renderItems = &mRenderItems;

    if (GResource::GUIManager->State.EnableModel) {
        for (auto &item : renderItems->at(EntityType::Opaque)) {
            item.DrawItem(cmdList, constantAddress);
        }
    }
}

void Scene::RenderQuad(ID3D12GraphicsCommandList *cmdList, D3D12_GPU_VIRTUAL_ADDRESS constantAddress, RenderItemsMap *renderItems)
{
    if (renderItems == nullptr) renderItems = &mRenderItems;
    for (auto &item : renderItems->at(EntityType::Quad)) {
        item.DrawItem(cmdList, constantAddress);
    }
}

void Scene::RenderScreenQuad(ID3D12GraphicsCommandList *cmdList, D3D12_VERTEX_BUFFER_VIEW *vertexBufferView, D3D12_INDEX_BUFFER_VIEW *indexBufferView)
{
    cmdList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    cmdList->IASetVertexBuffers(0, 1, vertexBufferView);
    cmdList->IASetIndexBuffer(indexBufferView);
    cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void Scene::RenderSphere(ID3D12GraphicsCommandList *cmdList, D3D12_GPU_VIRTUAL_ADDRESS constantAddress, RenderItemsMap *renderItems)
{
    if (renderItems == nullptr) renderItems = &mRenderItems;
    if (GResource::GUIManager->State.EnableSphere) {
        for (auto &item : renderItems->at(EntityType::Test)) {
            item.DrawItem(cmdList, constantAddress);
        }
    }
}

void Scene::CreateSphereTest(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList)
{
    auto gen = GeometryGenerator();
    auto mesh = gen.CreateSphere(2, 30, 30);
    std::vector<ModelVertex> vertices(mesh.Vertices.size());
    std::vector<uint16> indices;

    for (uint i = 0; i < mesh.Vertices.size(); i++) {
        vertices[i].Position = mesh.Vertices[i].Position;
        vertices[i].Normal = mesh.Vertices[i].Normal;
        vertices[i].UV = mesh.Vertices[i].TexC;
    }
    indices.insert(indices.end(), mesh.GetIndices16().begin(), mesh.GetIndices16().end());

    Mesh meshData{vertices, indices};
    auto meshInfo = AddMesh(meshData, device, cmdList);

    Entity entity(EntityType::Test);
    entity.MeshInfo = meshInfo;
    entity.ShaderID = static_cast<uint>(ShaderID::Opaque);
    entity.Transform = MathHelper::Identity4x4();
    entity.MaterialIndex = 0;
    entity.EntityIndex = mEntities.size();
    mEntities.push_back(entity);
}

void Scene::CreateQuadTest(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList)
{
    const std::vector<ModelVertex> vertices{{
        {{-1.0F, 1.0F, 0.0F}, {}, {0.0F, 0.0F}},
        {{1.0F, 1.0F, 0.0F}, {}, {1.0F, 0.0F}},
        {{-1.0F, -1.0F, 0.0F}, {}, {0.0F, 1.0F}},
        {{1.0F, -1.0F, 0.0F}, {}, {1.0F, 1.0F}},
    }};

    const std::vector<uint16> indices{{
        0,
        1,
        2,
        1,
        3,
        2,
    }};

    Mesh meshData{vertices, indices};
    auto meshInfo = AddMesh(meshData, device, cmdList);

    Entity entity(EntityType::Quad);
    entity.MeshInfo = meshInfo;
    entity.ShaderID = static_cast<uint>(ShaderID::Opaque);
    entity.Transform = MathHelper::Identity4x4();
    entity.MaterialIndex = 0;
    entity.EntityIndex = mEntities.size();
    mEntities.push_back(entity);
}

void Scene::LoadAssets(ID3D12Device *device, ID3D12GraphicsCommandList *commandList, DescriptorHeap *descriptorHeap, DataLoader *dataLoader, std::vector<Texture> &textures)
{
    // DataLoader light, materials, obj model(vertex index normal)
    uint repeat = GResource::config["RepeatModel"].as<uint>();
    mTransforms = dataLoader->GetTransforms(repeat);
    // CreateSceneInfo(dataLoader->GetLight()); // TODO CreateSceneInfo
    CreateMaterials(dataLoader->GetMaterials(), device, commandList, descriptorHeap, textures);
    mMeshesData = dataLoader->GetMeshes();
    CreateModels(dataLoader->GetModels(), device, commandList, repeat);
}

void Scene::CreateMaterials(const std::vector<ModelMaterial> &info,
                            ID3D12Device *device,
                            ID3D12GraphicsCommandList *commandList,
                            DescriptorHeap *descriptorHeap,
                            std::vector<Texture> &textures)
{
    int index = 0;

    // build Texture
    for (const auto &item : info) {
        Material material = {};
        material.BaseColor = item.basecolor;
        material.Index = index;
        material.Shineness = item.shiniess;
        if (item.diffuse_map != "null") { // TODO fix null bug
            std::wstring path = string2wstring(mRootPath + "\\" + item.diffuse_map);
            std::replace(path.begin(), path.end(), '/', '\\');
            Texture texture(device, commandList, path);
            uint index = textures.size();
            textures.push_back(std::move(texture));
            uint srvIndex = descriptorHeap->AddSrvDescriptor(device, textures[index].Resource());

            material.DiffuseMapIndex = srvIndex;
        }
        mMaterials.push_back(std::move(material));
        index++;
    }
    // build Materials Resouce
}

MeshInfo Scene::AddMesh(Mesh &mesh, ID3D12Device *device, ID3D12GraphicsCommandList *commandList)
{
    uint vertexOffset = mAllVertices.size();
    uint indexOffset = mAllIndices.size();
    uint vertexCount = mesh.Vertices.size();
    uint indexCount = mesh.Indices.size();

    for (const auto &Vertice : mesh.Vertices) {
        mAllVertices.push_back(Vertice);
    }
    for (const auto &Indice : mesh.Indices) {
        mAllIndices.push_back(Indice);
    }

    return {vertexOffset, vertexCount, indexOffset, indexCount};
}

void Scene::CreateModels(std::vector<Model> info, ID3D12Device *device, ID3D12GraphicsCommandList *commandList, uint repeat)
{
    for (uint i = 0; i < repeat + 1; i++) {
        for (const auto &item : info) {
            Entity entity(EntityType::Opaque);
            entity.Transform = mTransforms[item.transform+ i * info.size()];
            entity.MaterialIndex = item.material;
            entity.EntityIndex = mEntities.size();
            if (mSceneName == "common") {
                entity.ShaderID = static_cast<uint>(ShaderID::Opaque);
            } else {
                entity.ShaderID = static_cast<uint>(ShaderID::OpaqueWithTexture);
            }
            auto meshInfo = AddMesh(mMeshesData[item.mesh], device, commandList);
            entity.MeshInfo = meshInfo;
            mEntities.push_back(entity);
        }
    }
}

void Scene::BuildVertex2Constant(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList, UploadBuffer<ModelVertex> *verticesBuffer, UploadBuffer<uint16> *indicesBuffer)
{
    // Upload Vertex Index Data

    verticesBuffer->resource()->SetName(L"Scene Vertex Buffer");
    indicesBuffer->resource()->SetName(L"Scene Index Buffer");

    verticesBuffer->copyAllData(mAllVertices.data(), mAllVertices.size());
    indicesBuffer->copyAllData(mAllIndices.data(), mAllIndices.size());

    // Create Render Item
    for (const auto &item : mEntities) {
        RenderItem target;
        target
            .SetVertexInfo(item.MeshInfo.VertexOffset,
                           verticesBuffer->resource()->GetGPUVirtualAddress(),
                           sizeof(ModelVertex),
                           item.MeshInfo.VertexCount)
            .SetIndexInfo(item.MeshInfo.IndexOffset,
                          indicesBuffer->resource()->GetGPUVirtualAddress(),
                          sizeof(uint16),
                          item.MeshInfo.IndexCount)
            .SetConstantInfo(item.EntityIndex,
                             sizeof(EntityInfo),
                             0);
        switch (item.Type()) {
        case EntityType::Opaque:
            mRenderItems[EntityType::Opaque].push_back(target);
            break;
        case EntityType::Test:
            mRenderItems[EntityType::Test].push_back(target);
            break;
        case EntityType::Sky:
        case EntityType::Debug:
            break;
        case EntityType::Quad:
            mRenderItems[EntityType::Quad].push_back(target);
            break;
        }
    }
}

void Scene::UpdateSceneConstant(UploadBuffer<SceneInfo> *uploader)
{
    using DirectX::XMMATRIX;
    using DirectX::XMFLOAT4X4;
    mCamera.at("default").UpdateViewMatrix();
    SceneInfo sceneInfo = {};
    XMMATRIX view = mCamera["default"].GetView();
    XMMATRIX project = mCamera["default"].GetProj();
    XMMATRIX viewProject = DirectX::XMMatrixMultiply(view, project);

    XMFLOAT4X4 screenModel = {
        2 / static_cast<float>(GResource::Width), 0, 0, 0,
        0, -2 / static_cast<float>(GResource::Height), 0, 0,
        0, 0, 1, 0,
        -1, 1, 0, 1};
    XMMATRIX screenModelMat = DirectX::XMLoadFloat4x4(&screenModel);

    auto viewDet = XMMatrixDeterminant(view);
    auto projectDet = XMMatrixDeterminant(project);
    auto viewProjectDet = DirectX::XMMatrixDeterminant(viewProject);

    auto invView = XMMatrixInverse(&viewDet, view);
    auto invProject = XMMatrixInverse(&projectDet, project);
    auto invViewProject = XMMatrixInverse(&viewProjectDet, viewProject);

    XMStoreFloat4x4(&sceneInfo.View, XMMatrixTranspose(view));
    XMStoreFloat4x4(&sceneInfo.InvView, XMMatrixTranspose(invView));
    XMStoreFloat4x4(&sceneInfo.Project, XMMatrixTranspose(project));
    XMStoreFloat4x4(&sceneInfo.InvProject, XMMatrixTranspose(invProject));
    XMStoreFloat4x4(&sceneInfo.ViewProject, XMMatrixTranspose(viewProject));
    XMStoreFloat4x4(&sceneInfo.InvViewProject, XMMatrixTranspose(invViewProject));
    XMStoreFloat4x4(&sceneInfo.InvScreenModel, XMMatrixTranspose(screenModelMat));
    XMStoreFloat4(&sceneInfo.Ambient, DirectX::Colors::DarkGray);

    sceneInfo.EyePosition = mCamera["default"].GetPosition3f();

    uploader->copyData(0, sceneInfo);
}

void Scene::UpdateLightConstant(UploadBuffer<Light> *uploader)
{
    Light pointLight;
    pointLight.LightBegin = 0.0F;
    pointLight.LightEnd = 100.0F;
    pointLight.Position = DirectX::XMFLOAT3(3, 3, -5);
    pointLight.LightStrengh = DirectX::XMFLOAT3(1.0, 1.0, 1.0);
    uploader->copyData(0, pointLight);
}

void Scene::UpdateMaterialConstant(UploadBuffer<MaterialInfo> *uploader)
{
    for (uint i = 0; i < mMaterials.size(); i++) {
        MaterialInfo info = {};
        info.BaseColor = mMaterials[i].BaseColor;
        if (mMaterials[i].Shineness > 1.0F)
            info.Roughness = 0.02;
        else
            info.Roughness = 1 - mMaterials[i].Shineness;
        info.FresnelR0 = DirectX::XMFLOAT3(0.2F, 0.3F, 1.0F);
        info.DiffuseMapIndex = mMaterials[i].DiffuseMapIndex;
        uploader->copyData(i, info);
    }
}

void Scene::UpdateEntityConstant(UploadBuffer<EntityInfo> *uploader)
{
    using namespace DirectX;
    for (auto &entity : mEntities) {
        EntityInfo info;
        auto transform = DirectX::XMLoadFloat4x4(&entity.Transform);
        XMStoreFloat4x4(&info.Transform, DirectX::XMMatrixTranspose(transform));
        info.MaterialIndex = entity.MaterialIndex;
        info.ShaderID = entity.ShaderID;
        info.QuadType = static_cast<uint>(QuadShader::MixQuad);
        uploader->copyData(entity.EntityIndex, info);
    }
}

void Scene::UpdateCamera()
{
    const float deltaTime = 0.01F;
    if (GetAsyncKeyState('W') & 0x8000) {
        mCamera["default"].Walk(1.0F * deltaTime);
    }

    if (GetAsyncKeyState('S') & 0x8000) {
        mCamera["default"].Walk(-1.0F * deltaTime);
    }

    if (GetAsyncKeyState('A') & 0x8000) {
        mCamera["default"].Strafe(-1.0F * deltaTime);
    }

    if (GetAsyncKeyState('D') & 0x8000) {
        mCamera["default"].Strafe(1.0F * deltaTime);
    }

    mCamera["default"].UpdateViewMatrix();
}

void Scene::UpdateMouse(float dx, float dy)
{
    mCamera["default"].Pitch(dy);
    mCamera["default"].RotateY(dx);
}
void Scene::Reset()
{
    mTransforms.clear();
    mEntities.clear();
    mMaterials.clear();
    mRenderItems.clear();
    mAllVertices.clear();
    mAllIndices.clear();
}