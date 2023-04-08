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

void Scene::Init(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList)
{
    CreateCommonConstant(device);
    CreateSphereTest(device, cmdList);
    CreateQuadTest(device, cmdList);
    mDataLoader = std::make_unique<DataLoader>(mRootPath, mSceneName);
    LoadAssets(device, cmdList);
    BuildVertex2Constant(device, cmdList);
    CreateDescriptorHeaps2Descriptors(device, GResource::Width, GResource::Height);

    mCamera["default"].SetPosition(0, 0, -10.5F);
    mCamera["default"].SetLens(0.25f * MathHelper::Pi, GResource::Width / GResource::Height, 1, 1000);
}

void Scene::RenderScene(ID3D12GraphicsCommandList *cmdList)
{
    cmdList->SetGraphicsRootConstantBufferView(1, mSceneConstant->resource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootShaderResourceView(2, mLightConstant->resource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootShaderResourceView(5, mMaterialSR->resource()->GetGPUVirtualAddress());
    if (GResource::GUIManager->State.EnableModel) {
        for (auto &item : mRenderItems[EntityType::Opaque]) {
            item.DrawItem(cmdList);
        }
    }
}

void Scene::RenderQuad(ID3D12GraphicsCommandList *cmdList)
{
    for (auto &item : mRenderItems[EntityType::Quad]) {
        item.DrawItem(cmdList);
    }
}

void Scene::RenderSphere(ID3D12GraphicsCommandList *cmdList)
{
    cmdList->SetGraphicsRootConstantBufferView(1, mSceneConstant->resource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootShaderResourceView(2, mLightConstant->resource()->GetGPUVirtualAddress());
    cmdList->SetGraphicsRootShaderResourceView(5, mMaterialSR->resource()->GetGPUVirtualAddress());
    if (GResource::GUIManager->State.EnableSphere) {
        for (auto &item : mRenderItems[EntityType::Test]) {
            item.DrawItem(cmdList);
        }
    }
}

void Scene::UpdateScene()
{
    UpdateCamera();
    UpdateSceneConstant();
    UpdateEntityConstant();
    UpdateLight();
    UpdateMaterial();
}

std::array<CD3DX12_STATIC_SAMPLER_DESC, 7> Scene::GetStaticSamplers()
{
    const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
        0,                                // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_POINT,   // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

    const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
        1,                                 // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_POINT,    // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

    const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
        2,                                // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,  // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

    const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
        3,                                 // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_LINEAR,   // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

    const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
        4,                               // shaderRegister
        D3D12_FILTER_ANISOTROPIC,        // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressW
        0.0f,                            // mipLODBias
        8);                              // maxAnisotropy

    const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
        5,                                // shaderRegister
        D3D12_FILTER_ANISOTROPIC,         // filter
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressU
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressV
        D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressW
        0.0f,                             // mipLODBias
        8);                               // maxAnisotropy

    const CD3DX12_STATIC_SAMPLER_DESC shadow(
        6,                                                // shaderRegister
        D3D12_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, // filter
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,                // addressU
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,                // addressV
        D3D12_TEXTURE_ADDRESS_MODE_BORDER,                // addressW
        0.0f,                                             // mipLODBias
        16,                                               // maxAnisotropy
        D3D12_COMPARISON_FUNC_LESS_EQUAL,
        D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK);

    return {
        pointWrap, pointClamp,
        linearWrap, linearClamp,
        anisotropicWrap, anisotropicClamp,
        shadow};
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
void Scene::CreateCommonConstant(ID3D12Device *device)
{
    mSceneConstant = std::make_unique<UploadBuffer<SceneInfo>>(device, 1, true);
    mLightConstant = std::make_unique<UploadBuffer<Light>>(device, 1, false);
}

void Scene::CreateDescriptorHeaps2Descriptors(ID3D12Device *device, uint width, uint height)
{
    // SRV
    mSRVDescriptorHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, mMaterials.size(), true);
    int index = 0;
    for (const auto &item : mMaterials) {
        // TODO 更定Material 和 Texture ID的问题因为存在Null Texture
        if (item.Texture == nullptr) continue;
        auto resourceDesc = item.Texture->GetDesc();
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

        srvDesc.Format = resourceDesc.Format;
        if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D) {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;
            srvDesc.Texture2D.MostDetailedMip = 0;
            srvDesc.Texture2D.ResourceMinLODClamp = 0.0F;
        }
        if (resourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE3D) {
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
            srvDesc.TextureCube.MipLevels = resourceDesc.MipLevels;
            srvDesc.TextureCube.ResourceMinLODClamp = 0.0F;
            srvDesc.TextureCube.MostDetailedMip = 0;
        }

        auto handle = mSRVDescriptorHeap->CPUHandle(index);
        device->CreateShaderResourceView(item.Texture->Resource(),
                                         &srvDesc,
                                         handle);
        index++;
    }
}

void Scene::LoadAssets(ID3D12Device *device, ID3D12GraphicsCommandList *commandList)
{
    // DataLoader light, materials, obj model(vertex index normal)
    mTransforms = mDataLoader->GetTransforms();
    // CreateSceneInfo(mDataLoader->GetLight()); // TODO CreateSceneInfo
    CreateMaterials(mDataLoader->GetMaterials(), device, commandList);
    mMeshesData = mDataLoader->GetMeshes();
    CreateModels(mDataLoader->GetModels(), device, commandList);
}

void Scene::CreateMaterials(const std::vector<ModelMaterial> &info,
                            ID3D12Device *device,
                            ID3D12GraphicsCommandList *commandList)
{
    int index = 0;

    // build Texture
    for (const auto &item : info) {
        Material material = {};
        material.BaseColor = item.basecolor;
        material.Index = index;
        material.Shineness = item.shiniess;
        if (item.diffuse_map != "null") {
            std::wstring path = string2wstring(mRootPath + "\\" + item.diffuse_map);
            std::replace(path.begin(), path.end(), '/', '\\');
            Texture texture(device, commandList, path);
            uint index = GResource::TextureManager->StoreTexture(texture);
            uint srvIndex = GResource::TextureManager->AddSrvDescriptor(index);
            GResource::TextureManager->SetDescriptorName(item.diffuse_map, srvIndex);
        }
        mMaterials.push_back(std::move(material));
        index++;
    }

    // build Materials Resouce
    mMaterialSR = std::make_unique<UploadBuffer<MaterialInfo>>(device, mMaterials.size(), false);
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

void Scene::CreateModels(std::vector<Model> info, ID3D12Device *device, ID3D12GraphicsCommandList *commandList)
{
    for (const auto &item : info) {
        Entity entity(EntityType::Opaque);
        entity.Transform = mTransforms[item.transform];
        entity.MaterialIndex = item.material;
        entity.EntityIndex = mEntities.size();
        entity.ShaderID = static_cast<uint>(ShaderID::Opaque);
        auto meshInfo = AddMesh(mMeshesData[item.mesh], device, commandList);
        entity.MeshInfo = meshInfo;
        mEntities.push_back(entity);
    }
}

void Scene::BuildVertex2Constant(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList)
{
    // Upload Vertex Index Data
    mVerticesBuffer = std::make_unique<UploadBuffer<ModelVertex>>(device, mAllVertices.size(), false);
    mIndicesBuffer = std::make_unique<UploadBuffer<UINT16>>(device, mAllIndices.size(), false);
    mVerticesBuffer->copyAllData(mAllVertices.data(), mAllVertices.size());
    mIndicesBuffer->copyAllData(mAllIndices.data(), mAllIndices.size());

    // ConstantData Common Entity +  Test Entity
    mObjectConstant = std::make_unique<UploadBuffer<EntityInfo>>(device, mEntities.size(), true);
    for (const auto &item : mEntities) {
        EntityInfo entityInfo = {};
        entityInfo.Transform = item.Transform;
        entityInfo.MaterialIndex = item.MaterialIndex;
        entityInfo.ShaderID = item.ShaderID;
        mObjectConstant->copyData(item.EntityIndex, entityInfo);
    }

    // Create Render Item
    for (const auto &item : mEntities) {
        RenderItem target;
        target
            .SetVertexInfo(item.MeshInfo.VertexOffset,
                           mVerticesBuffer->resource()->GetGPUVirtualAddress(),
                           sizeof(ModelVertex),
                           item.MeshInfo.VertexCount)
            .SetIndexInfo(item.MeshInfo.IndexOffset,
                          mIndicesBuffer->resource()->GetGPUVirtualAddress(),
                          sizeof(uint16),
                          item.MeshInfo.IndexCount)
            .SetConstantInfo(item.EntityIndex,
                             mObjectConstant->resource()->GetGPUVirtualAddress(),
                             sizeof(EntityInfo),
                             0);
        switch (item.Type()) {
        case EntityType::Opaque:
            mRenderItems[EntityType::Opaque].push_back(target);
            break;
        case EntityType::Test:
            mRenderItems[EntityType::Test].push_back(target);
            break;
        }
    }
}

void Scene::UpdateSceneConstant()
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

    mSceneConstant->copyData(0, sceneInfo);
}

void Scene::UpdateLight()
{
    Light pointLight;
    pointLight.LightBegin = 0.0F;
    pointLight.LightEnd = 100.0F;
    pointLight.Position = DirectX::XMFLOAT3(3, 3, -5);
    pointLight.LightStrengh = DirectX::XMFLOAT3(1.0, 1.0, 1.0);
    mLightConstant->copyData(0, pointLight);
}

void Scene::UpdateMaterial()
{
    for (uint i = 0; i < mMaterials.size(); i++) {
        MaterialInfo info = {};
        info.Diffuse = mMaterials[i].BaseColor;
        // info.Roughness = 1 - mMaterials[i].Shineness; // TODO 默认Roughness是非归一化的数值如32
        info.Roughness = 0.02;
        info.FresnelR0 = DirectX::XMFLOAT3(0.2F, 0.3F, 1.0F);
        mMaterialSR->copyData(i, info);
    }
}

void Scene::UpdateEntityConstant()
{
    using namespace DirectX;
    for (auto &entity : mEntities) {
        EntityInfo info;
        auto transform = DirectX::XMLoadFloat4x4(&entity.Transform);
        XMStoreFloat4x4(&info.Transform, DirectX::XMMatrixTranspose(transform));
        info.MaterialIndex = entity.MaterialIndex;
        info.ShaderID = static_cast<uint>(ShaderID::Opaque);
        mObjectConstant->copyData(entity.EntityIndex, info);
    }
    // Sphere
    EntityInfo info;
    auto transform = XMMatrixIdentity();
    XMStoreFloat4x4(&info.Transform, DirectX::XMMatrixTranspose(transform));
    info.MaterialIndex = 1; // TODO Model 1# 材质
    info.ShaderID = static_cast<uint>(ShaderID::Opaque);
    mObjectConstant->copyData(2, info);
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
