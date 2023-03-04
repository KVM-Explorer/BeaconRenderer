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
    mRTVDescriptorHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, frameCount);
    for (UINT i = 0; i < frameCount; i++) {
        ComPtr<ID3D12Resource> buffer;
        auto handle = mRTVDescriptorHeap->CPUHandle(i);
        ThrowIfFailed(swapChain->GetBuffer(i, IID_PPV_ARGS(&buffer)));
        device->CreateRenderTargetView(buffer.Get(), nullptr, handle);
        mRTVBuffer.push_back(std::move(buffer));
    }
}

void Scene::CreateInputLayout()
{
    mInputLayout["Test"] = {{
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

    mInputLayout["Model"] = {{{"POSITION",
                               0,
                               DXGI_FORMAT_R32G32B32_FLOAT,
                               0,
                               0,
                               D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                               0},
                              {"TEXCOORD",
                               0,
                               DXGI_FORMAT_R32G32_FLOAT,
                               0,
                               12,
                               D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                               0},
                              {"NORMAL",
                               0,
                               DXGI_FORMAT_R32G32_FLOAT,
                               0,
                               0,
                               D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                               20}}};
}

void Scene::CompileShaders()
{
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

    ThrowIfFailed(D3DCompileFromFile(L"Shaders/Test.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &mShaders["TestVS"], nullptr));
    ThrowIfFailed(D3DCompileFromFile(L"Shaders/Test.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &mShaders["TestPS"], nullptr));

    ThrowIfFailed(D3DCompileFromFile(L"Shaders/Model.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &mShaders["ModelVS"], nullptr));
    ThrowIfFailed(D3DCompileFromFile(L"Shaders/Model.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &mShaders["ModelPS"], nullptr));
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
                                              IID_PPV_ARGS(&mSignature["Test"])));

    std::array<CD3DX12_ROOT_PARAMETER, 4> rootParameters;
    rootParameters.at(0).InitAsConstantBufferView(0);
    rootParameters.at(1).InitAsConstantBufferView(1);
    rootParameters.at(2).InitAsShaderResourceView(0, 1);
    rootParameters.at(3).InitAsConstantBufferView(0, 0);

    auto samplers = GetStaticSamplers();

    CD3DX12_ROOT_SIGNATURE_DESC commonRootSignatureDesc(rootParameters.size(),
                                                        rootParameters.data(),
                                                        samplers.size(),
                                                        samplers.data(),
                                                        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    ThrowIfFailed(D3D12SerializeRootSignature(&commonRootSignatureDesc,
                                              D3D_ROOT_SIGNATURE_VERSION_1_0,
                                              &signature, &error));
    ThrowIfFailed(device->CreateRootSignature(0,
                                              signature->GetBufferPointer(),
                                              signature->GetBufferSize(),
                                              IID_PPV_ARGS(&mSignature["Model"])));
}

void Scene::CreatePipelineStateObject(ID3D12Device *device)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

    psoDesc.InputLayout = {mInputLayout["Test"].data(),
                           static_cast<UINT>(mInputLayout["Test"].size())};
    psoDesc.pRootSignature = mSignature["Test"].Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(mShaders["TestVS"].Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(mShaders["TestPS"].Get());
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

    ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSO["Test"])));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC opaqueDesc = {};
    opaqueDesc.VS = CD3DX12_SHADER_BYTECODE(mShaders["ModelVS"].Get());
    opaqueDesc.PS = CD3DX12_SHADER_BYTECODE(mShaders["ModelPS"].Get());
    opaqueDesc.InputLayout = {mInputLayout["Model"].data(),
                              static_cast<UINT>(mInputLayout["Model"].size())};
    opaqueDesc.pRootSignature = mSignature["Model"].Get();
    opaqueDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    opaqueDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    opaqueDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    opaqueDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    opaqueDesc.NumRenderTargets = 1;
    opaqueDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    opaqueDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
    opaqueDesc.SampleDesc.Count = 1;
    opaqueDesc.SampleMask = UINT_MAX;
    ThrowIfFailed(device->CreateGraphicsPipelineState(&opaqueDesc, IID_PPV_ARGS(&mPSO["Model"])));
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
    mVerticesBuffer = std::make_unique<UploadBuffer<ModelVertex>>(device, mAllVertices.size(), false);
    mIndicesBuffer = std::make_unique<UploadBuffer<UINT16>>(device, mAllIndices.size(), false);
    mVerticesBuffer->copyAllData(mAllVertices.data(), mAllVertices.size());
    mIndicesBuffer->copyAllData(mAllIndices.data(), mAllIndices.size());

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
                           mVerticesBuffer->resource()->GetGPUVirtualAddress(),
                           sizeof(ModelVertex),
                           item.MeshInfo.VertexCount)
            .SetIndexInfo(item.MeshInfo.IndexOffset,
                          mIndicesBuffer->resource()->GetGPUVirtualAddress(),
                          sizeof(uint16),
                          item.MeshInfo.IndexCount)
            .SetConstantInfo(item.EntityIndex,
                             mObjectConstant->resource()->GetGPUVirtualAddress(),
                             sizeof(DirectX::XMFLOAT4X4),
                             0); // TODO Update Root Parameter Index
    }
}

void Scene::RenderTriangleScene(ID3D12GraphicsCommandList *commandList, uint frameIndex)
{
    commandList->SetGraphicsRootSignature(mSignature["Test"].Get());
    commandList->SetPipelineState(mPSO["Test"].Get());

    // State Convert Befor Render Barrier
    auto beginBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mRTVBuffer.at(frameIndex).Get(),
                                                             D3D12_RESOURCE_STATE_PRESENT,
                                                             D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &beginBarrier);

    // Rendering
    auto rtvHandle = mRTVDescriptorHeap->CPUHandle(frameIndex);

    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvHandle, DirectX::Colors::SteelBlue, 0, nullptr);
    commandList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    commandList->IASetVertexBuffers(0, 1, &mVertexBufferView);
    commandList->DrawInstanced(3, 1, 0, 0);

    // State Convert After Render Barrier
    auto endBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mRTVBuffer.at(frameIndex).Get(),
                                                           D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                           D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(1, &endBarrier);
}

void Scene::RenderModelScene(ID3D12GraphicsCommandList *commandList, uint frameIndex)
{
    // TODO Update Model Scene Parameters
    commandList->SetGraphicsRootSignature(mSignature["Model"].Get());
    commandList->SetPipelineState(mPSO["Model"].Get());

    // State Convert Befor Render Barrier
    auto beginBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mRTVBuffer.at(frameIndex).Get(),
                                                             D3D12_RESOURCE_STATE_PRESENT,
                                                             D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &beginBarrier);

    // Rendering
    auto rtvHandle = mRTVDescriptorHeap->CPUHandle(frameIndex);

    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
    commandList->ClearRenderTargetView(rtvHandle, DirectX::Colors::SteelBlue, 0, nullptr);

    for (auto &item : mRenderItems[EntityType::Opaque]) {
        item.DrawItem(commandList);
    }

    // State Convert After Render Barrier
    auto endBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mRTVBuffer.at(frameIndex).Get(),
                                                           D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                           D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(1, &endBarrier);
}