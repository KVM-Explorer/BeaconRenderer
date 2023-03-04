#include "Scene.h"
#include "Tools/DataLoader.h"
#include "Tools/FrameworkHelper.h"

Scene::Scene(const std::wstring &root, const std::wstring &scenename) :
    mRootPath(root),
    mSceneName(scenename)
{
}

void Scene::Init(SceneAdapter &adapter)
{
    CreateRTV(adapter.Device, adapter.SwapChain, adapter.FrameCount);
    CreateInputLayout();
    CompileShaders();
    CreateRootSignature(adapter.Device);
    CreatePipelineStateObject(adapter.Device);
    CreateTriangleVertex(adapter.Device, adapter.CommandList);
    CreateCommonConstant();
    mDataLoader = std::make_unique<DataLoader>(mRootPath, mSceneName);
    LoadAssets(adapter.Device, adapter.CommandList);
}

void Scene::RenderScene(ID3D12GraphicsCommandList *commandList, uint frameIndex)
{
    RenderTriangleScene(commandList, frameIndex);
}

void Scene::RenderUI()
{
}

void Scene::CreateRTV(ID3D12Device *device, IDXGISwapChain *swapChain, uint frameCount)
{
    RTVDescriptorHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, frameCount);
    for (UINT i = 0; i < frameCount; i++) {
        ComPtr<ID3D12Resource> buffer;
        auto handle = RTVDescriptorHeap->CPUHandle(i);
        ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&buffer)));
        device->CreateRenderTargetView(buffer.Get(), nullptr, handle);
        RTVBuffer.push_back(std::move(buffer));
    }
}

void Scene::CreateInputLayout()
{
    mInputLayout = {{
        {"POSITION",
         0,
         DXGI_FORMAT_R32G32B32_FLOAT,
         0,
         0,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
         0},
        {"COLOR",
         0,
         DXGI_FORMAT_R32G32B32A32_FLOAT,
         0,
         12,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
         0},
    }};
}

void Scene::CompileShaders()
{
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

    ThrowIfFailed(D3DCompileFromFile(L"Shaders/default.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &mShaders["defaultVS"], nullptr));
    ThrowIfFailed(D3DCompileFromFile(L"Shaders/default.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &mShaders["defaultPS"], nullptr));
}



void Scene::CreateRootSignature(ID3D12Device *device)
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(0,
                                                  nullptr,
                                                  0,
                                                  nullptr,
                                                  D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    ComPtr<ID3DBlob> signature = nullptr;
    ComPtr<ID3DBlob> error = nullptr;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc,
                                              D3D_ROOT_SIGNATURE_VERSION_1_0,
                                              &signature, &error));
    ThrowIfFailed(device->CreateRootSignature(0,
                                              signature->GetBufferPointer(),
                                              signature->GetBufferSize(),
                                              IID_PPV_ARGS(&mSignature["default"])));

    std::array<CD3DX12_ROOT_PARAMETER, 4> rootParameters;
    rootParameters.at(0).InitAsConstantBufferView(0);
    rootParameters.at(1).InitAsConstantBufferView(1);
    rootParameters.at(2).InitAsShaderResourceView(0, 1);

    auto samplers = CreateStaticSampler();

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(rootParameters.size(),
                                                  rootParameters.data(),
                                                  samplers.size(),
                                                  samplers.data(),
                                                  D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
}

void Scene::CreatePipelineStateObject(ID3D12Device *device)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

    psoDesc.InputLayout = {mInputLayout.data(), static_cast<UINT>(mInputLayout.size())};
    psoDesc.pRootSignature = mSignature["default"].Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(mShaders["defaultVS"].Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(mShaders["defaultPS"].Get());
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;

    psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
    psoDesc.BlendState.IndependentBlendEnable = FALSE;
    psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    psoDesc.DepthStencilState.DepthEnable = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;

    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.NumRenderTargets = 1;

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.SampleDesc.Count = 1;

    ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO["default"])));
}

void Scene::CreateTriangleVertex(ID3D12Device *device, ID3D12GraphicsCommandList *commandList)
{
    const float triangleSize = 2.0F;
    const std::array<Vertex, 3> vertexData = {{
        {{-0.25F * triangleSize, -0.25F * triangleSize, 0.F}, {1.F, 0.F, 0.F, 1.F}},
        {{0.0F, 0.25F * triangleSize, 0.0F}, {0.0F, 1.0F, 0.0F, 0.0F}},
        {{0.25F * triangleSize, -0.25F * triangleSize, 0.F}, {0.F, 0.F, 1.F, 0.F}},
    }};
    const uint vertexByteSize = vertexData.size() * sizeof(Vertex);
    mVertexBuffer = std::make_unique<DefaultBuffer>(device, commandList, vertexData.data(), vertexByteSize);

    mVertexBufferView.BufferLocation = mVertexBuffer->Resource()->GetGPUVirtualAddress();
    mVertexBufferView.SizeInBytes = vertexByteSize;
    mVertexBufferView.StrideInBytes = sizeof(Vertex);
}

void Scene::CreateCommonConstant()
{
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
    for (const auto &item : info) {
        Material material = {};
        material.BaseColor = item.basecolor;
        material.Index = index;
        material.Shineness = item.shiniess;
        if (item.diffuse_map != "null") {
            std::wstring path = mRootPath + L"\\" + string2wstring(item.diffuse_map);
            std::replace(path.begin(), path.end(), '/', '\\');
            material.Texture = std::make_unique<Texture>(device, commandList, path);
        }
        mMaterials.push_back(std::move(material));
        index++;
    }
}

MeshInfo Scene::CreateMeshes(Mesh &mesh, ID3D12Device *device, ID3D12GraphicsCommandList *commandList)
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
        entity.EntityIndex = mEntityCount;
        auto meshInfo = CreateMeshes(mMeshesData[item.mesh], device, commandList);
        entity.MeshInfo = meshInfo;
        mEntityCount++;
        mEntities.push_back(entity);
    }

    // Upload Data
    VerticesBuffer = std::make_unique<UploadBuffer<ModelVertex>>(device, mAllVertices.size(), false);
    IndicesBuffer = std::make_unique<UploadBuffer<UINT16>>(device, mAllIndices.size(), false);
    VerticesBuffer->copyAllData(mAllVertices.data(), mAllVertices.size());
    IndicesBuffer->copyAllData(mAllIndices.data(), mAllIndices.size());

    // ConstantData
    mObjectConstant = std::make_unique<UploadBuffer<EntityInfo>>(device, mEntities.size(), true);
    for (const auto &item : mEntities) {
        EntityInfo entityInfo = {};
        entityInfo.MaterialIndex = item.MaterialIndex;
        entityInfo.Transform = item.Transform;
        mObjectConstant->copyData(item.EntityIndex, entityInfo);
    }

    // Create Render Item
    for (const auto &item : mEntities) {
        RenderItem target;
        target
            .SetVertexInfo(item.MeshInfo.VertexOffset,
                           VerticesBuffer->resource()->GetGPUVirtualAddress(),
                           sizeof(ModelVertex),
                           item.MeshInfo.VertexCount)
            .SetIndexInfo(item.MeshInfo.IndexOffset,
                          IndicesBuffer->resource()->GetGPUVirtualAddress(),
                          sizeof(uint16),
                          item.MeshInfo.IndexCount)
            .SetConstantInfo(item.EntityIndex,
                             mObjectConstant->resource()->GetGPUVirtualAddress(),
                             sizeof(DirectX::XMFLOAT4X4),
                             3); // TODO Update Root Parameter Index
    }
}

void Scene::RenderTriangleScene(ID3D12GraphicsCommandList *commandList, uint frameIndex)
{
    commandList->SetGraphicsRootSignature(mSignature["default"].Get());
    commandList->SetPipelineState(mPSO["default"].Get());

    // State Convert Befor Render Barrier
    auto beginBarrier = CD3DX12_RESOURCE_BARRIER::Transition(RTVBuffer.at(frameIndex).Get(),
                                                             D3D12_RESOURCE_STATE_PRESENT,
                                                             D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &beginBarrier);

    // Rendering
    auto rtvHandle = RTVDescriptorHeap->CPUHandle(frameIndex);

    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvHandle, DirectX::Colors::SteelBlue, 0, nullptr);
    commandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    commandList->IASetVertexBuffers(0, 1, &mVertexBufferView);
    commandList->DrawInstanced(3, 1, 0, 0);

    // State Convert After Render Barrier
    auto endBarrier = CD3DX12_RESOURCE_BARRIER::Transition(RTVBuffer.at(frameIndex).Get(),
                                                           D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                           D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(1, &endBarrier);
}

void Scene::RenderModelScene(ID3D12GraphicsCommandList *commandList, uint frameIndex)
{
    // TODO Update Model Scene Parameters
    commandList->SetGraphicsRootSignature(mSignature["default"].Get());
    commandList->SetPipelineState(mPSO["default"].Get());

    // State Convert Befor Render Barrier
    auto beginBarrier = CD3DX12_RESOURCE_BARRIER::Transition(RTVBuffer.at(frameIndex).Get(),
                                                             D3D12_RESOURCE_STATE_PRESENT,
                                                             D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &beginBarrier);

    // Rendering
    auto rtvHandle = RTVDescriptorHeap->CPUHandle(frameIndex);

    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvHandle, DirectX::Colors::SteelBlue, 0, nullptr);

    for (auto &item : mRenderItems[EntityType::Opaque]) {
        item.DrawItem(commandList);
    }

    // State Convert After Render Barrier
    auto endBarrier = CD3DX12_RESOURCE_BARRIER::Transition(RTVBuffer.at(frameIndex).Get(),
                                                           D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                           D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(1, &endBarrier);
}