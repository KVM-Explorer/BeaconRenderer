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
    mDataLoader = std::make_unique<DataLoader>(mRootPath, mSceneName);
    LoadAssets(adapter.Device, adapter.CommandList);
}

void Scene::RenderScene(ID3D12GraphicsCommandList *commandList, uint frameIndex)
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

void Scene::LoadAssets(ID3D12Device *device, ID3D12GraphicsCommandList *commandList)
{
    // DataLoader light, materials, obj model(vertex index normal)
    mTransforms = mDataLoader->GetTransforms();
    // CreateSceneInfo(mDataLoader->GetLight()); // TODO CreateSceneInfo
    CreateMaterials(mDataLoader->GetMaterials(), device, commandList);
    mMeshesData = mDataLoader->GetMeshes();
    CreateMeshes(device, commandList);
    CreateModels(device, commandList);
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

void Scene::CreateMeshes(ID3D12Device *device, ID3D12GraphicsCommandList *commandList)
{
    // TODO CretaMeshes
}

void Scene::CreateModels(ID3D12Device *device, ID3D12GraphicsCommandList *commandList)
{
    // TODO CreateModels
}