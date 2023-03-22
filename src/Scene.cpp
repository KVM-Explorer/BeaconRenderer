#include "Scene.h"
#include "Tools/DataLoader.h"
#include "Tools/FrameworkHelper.h"
#include "Framework/Application.h"

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
    CreateDeferredRenderTriangle(adapter.Device, adapter.CommandList);
    CreateCommonConstant(adapter.Device);

    InitUI(adapter.Device);

    mDataLoader = std::make_unique<DataLoader>(mRootPath, mSceneName);
    LoadAssets(adapter.Device, adapter.CommandList);
    CreateDescriptorHeaps2Descriptors(adapter.Device, adapter.FrameWidth, adapter.FrameHeight);

    mCamera["default"].SetPosition(0, 0, -1.5F);
    mCamera["default"].SetLens(0.25f * MathHelper::Pi, adapter.FrameWidth / adapter.FrameHeight, 1, 1000);

    mDeferredRendering = std::make_unique<DeferredRendering>(adapter.FrameWidth, adapter.FrameHeight);
    mDeferredRendering->Init(adapter.Device);
    mDeferredRendering->CreatePSOs(adapter.Device,
                                   mInputLayout["GBuffer"], mShaders["GBufferVS"], mShaders["GBufferPS"],
                                   mInputLayout["LightPass"], mShaders["LightPassVS"], mShaders["LightPassPS"]);
}

void Scene::RenderScene(ID3D12GraphicsCommandList *commandList, uint frameIndex)
{
    // RenderTriangleScene(commandList, frameIndex);
    // RenderModelScene(commandList, frameIndex);
    DeferredRenderScene(commandList, frameIndex);
}

void Scene::RenderUI(ID3D12GraphicsCommandList *cmdList, uint frameIndex)
{
    // Define GUI
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    bool show_window = true;
    ImGui::Begin("Another Window", &show_window); // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
    ImGui::Text("Hello from another window!");

    ImGui::End();

    bool showDemo = true;
    ImGui::ShowDemoWindow(&showDemo);

    // Generate GUI
    ImGui::Render();

    // Render GUI To Screen
    std::array<ID3D12DescriptorHeap *, 1> ppHeaps{mUiSrvHeap->Resource()};
    cmdList->SetDescriptorHeaps(ppHeaps.size(), ppHeaps.data());
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdList);
    // State Convert After Render Barrier
    auto endBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mRTVBuffer.at(frameIndex).Get(),
                                                           D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                           D3D12_RESOURCE_STATE_PRESENT);
    cmdList->ResourceBarrier(1, &endBarrier);
}

void Scene::UpdateScene()
{
    UpdateCamera();
    UpdateSceneConstant();
    UpdateEntityConstant();
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

    mInputLayout["Model"] = {{
        {"POSITION",
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
         20,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
         0},
    }};

    mInputLayout["GBuffer"] = {{
        // TODO Fill Layout
        {"POSITION",
         0,
         DXGI_FORMAT_R32G32B32_FLOAT,
         0,
         0,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
         0},
        {"NORMAL",
         0,
         DXGI_FORMAT_R32G32B32_FLOAT,
         0,
         12,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
         0},
    }};
    mInputLayout["LightPass"] = {{
        {"POSITION",
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
    }};
}

void Scene::CompileShaders()
{
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;

    ThrowIfFailed(D3DCompileFromFile(L"Shaders/Test.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &mShaders["TestVS"], nullptr));
    ThrowIfFailed(D3DCompileFromFile(L"Shaders/Test.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &mShaders["TestPS"], nullptr));

    ThrowIfFailed(D3DCompileFromFile(L"Shaders/Model.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &mShaders["ModelVS"], nullptr));
    ThrowIfFailed(D3DCompileFromFile(L"Shaders/Model.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &mShaders["ModelPS"], nullptr));

    ThrowIfFailed(D3DCompileFromFile(L"Shaders/GBuffer.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &mShaders["GBufferVS"], nullptr));
    ThrowIfFailed(D3DCompileFromFile(L"Shaders/GBuffer.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &mShaders["GBufferPS"], nullptr));

    ThrowIfFailed(D3DCompileFromFile(L"Shaders/LightingPass.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &mShaders["LightPassVS"], nullptr));
    ThrowIfFailed(D3DCompileFromFile(L"Shaders/LightingPass.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &mShaders["LightPassPS"], nullptr));
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
    std::array<CD3DX12_ROOT_PARAMETER, 2> testRootParameters;
    testRootParameters.at(0).InitAsConstantBufferView(0);
    testRootParameters.at(1).InitAsConstantBufferView(1);
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc(testRootParameters.size(),
                                                  testRootParameters.data(),
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

    CD3DX12_DESCRIPTOR_RANGE textureRange;
    textureRange.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 28, 0, 1);

    std::array<CD3DX12_ROOT_PARAMETER, 3> rootParameters;
    rootParameters.at(0).InitAsConstantBufferView(0);             // Object
    rootParameters.at(1).InitAsConstantBufferView(1);             // Common
    rootParameters.at(2).InitAsDescriptorTable(1, &textureRange); // Material/Texture
    // rootParameters.at(3).InitAsConstantBufferView(0, 0); //

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

void Scene::CreateDeferredRenderTriangle(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList)
{
    const float triangleSize = 2.0F;
    const std::array<GBufferVertex, 3> vertexData = {{
        {{-0.25F * triangleSize, -0.25F * triangleSize, 0.F}, {0.F, 0.F, -1.0F}},
        {{0.0F, 0.25F * triangleSize, 0.0F}, {0.F, 0.F, -1.0F}},
        {{0.25F * triangleSize, -0.25F * triangleSize, 0.F}, {0.F, 0.F, -1.0F}},
    }};
    const uint gBuffervertexByteSize = vertexData.size() * sizeof(GBufferVertex);
    mGBufferVertexBuffer = std::make_unique<DefaultBuffer>(device, cmdList, vertexData.data(), gBuffervertexByteSize);

    mGBufferVertexView.BufferLocation = mGBufferVertexBuffer->Resource()->GetGPUVirtualAddress();
    mGBufferVertexView.SizeInBytes = gBuffervertexByteSize;
    mGBufferVertexView.StrideInBytes = sizeof(GBufferVertex);

    const std::array<LightPassVertex, 4> quadData{{
        {{-1.0F, 1.0F, 0.0F}, {0.0F, 0.0F}},
        {{1.0F, 1.0F, 0.0F}, {1.0F, 0.0F}},
        {{-1.0F, -1.0F, 0.0F}, {0.0F, 1.0F}},
        {{1.0F, -1.0F, 0.0F}, {1.0F, 1.0F}},
    }};

    const uint lightPassVertexByteSize = quadData.size() * sizeof(LightPassVertex);
    mQuadVertexBuffer = std::make_unique<DefaultBuffer>(device, cmdList, quadData.data(), lightPassVertexByteSize);

    mQuadVertexView.BufferLocation = mQuadVertexBuffer->Resource()->GetGPUVirtualAddress();
    mQuadVertexView.SizeInBytes = lightPassVertexByteSize;
    mQuadVertexView.StrideInBytes = sizeof(LightPassVertex);
}

void Scene::CreateCommonConstant(ID3D12Device *device)
{
    mSceneConstant = std::make_unique<UploadBuffer<SceneInfo>>(device, 1, true);
}

void Scene::CreateDescriptorHeaps2Descriptors(ID3D12Device *device, uint width, uint height)
{
    // DSV
    mDSVDescriptorHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
    D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilDesc = {};
    depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthStencilDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    depthStencilDesc.Flags = D3D12_DSV_FLAG_NONE;

    D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
    depthOptimizedClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthOptimizedClearValue.DepthStencil.Depth = 1.0F;
    depthOptimizedClearValue.DepthStencil.Stencil = 0.0F;

    CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC texture2D = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D24_UNORM_S8_UINT,
                                                                   width,
                                                                   height,
                                                                   1,
                                                                   1,
                                                                   1,
                                                                   0,
                                                                   D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    ThrowIfFailed(device->CreateCommittedResource(&heapProperties,
                                                  D3D12_HEAP_FLAG_NONE,
                                                  &texture2D,
                                                  D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                                  &depthOptimizedClearValue,
                                                  IID_PPV_ARGS(&mDepthStencilBuffer)));
    device->CreateDepthStencilView(mDepthStencilBuffer.Get(),
                                   &depthStencilDesc,
                                   mDSVDescriptorHeap->CPUHandle(0));

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
        mRenderItems[EntityType::Opaque].push_back(target);
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

    // Binding Heap
    std::array<ID3D12DescriptorHeap *, 1> textureHeap = {mSRVDescriptorHeap->Resource()};
    commandList->SetDescriptorHeaps(textureHeap.size(), textureHeap.data());
    commandList->SetGraphicsRootDescriptorTable(2, mSRVDescriptorHeap->GPUHandle(0));

    // Rendering
    auto rtvHandle = mRTVDescriptorHeap->CPUHandle(frameIndex);
    auto dsvHandle = mDSVDescriptorHeap->CPUHandle(0);

    commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, &dsvHandle);
    commandList->ClearRenderTargetView(rtvHandle, DirectX::Colors::SteelBlue, 0, nullptr);
    commandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0F, 0, 0, nullptr);

    commandList->SetGraphicsRootConstantBufferView(1, mSceneConstant->resource()->GetGPUVirtualAddress());

    for (auto &item : mRenderItems[EntityType::Opaque]) {
        item.DrawItem(commandList);
    }
}

void Scene::DeferredRenderScene(ID3D12GraphicsCommandList *cmdList, uint frameIndex)
{
    auto rtvHandle = mRTVDescriptorHeap->CPUHandle(frameIndex);
    auto present2rtvBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mRTVBuffer.at(frameIndex).Get(),
                                                                   D3D12_RESOURCE_STATE_PRESENT,
                                                                   D3D12_RESOURCE_STATE_RENDER_TARGET);

    mDeferredRendering->GBufferPass(cmdList);
    cmdList->SetGraphicsRootConstantBufferView(1, mSceneConstant->resource()->GetGPUVirtualAddress());

    // TODO achieve Deferred Render Scene
    cmdList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    cmdList->IASetVertexBuffers(0, 1, &mGBufferVertexView);
    cmdList->DrawInstanced(3, 1, 0, 0);
    //

    cmdList->ResourceBarrier(1, &present2rtvBarrier);
    cmdList->OMSetRenderTargets(1, &rtvHandle, true, nullptr);
    cmdList->ClearRenderTargetView(rtvHandle, DirectX::Colors::SteelBlue, 0, nullptr);
    mDeferredRendering->LightingPass(cmdList);

    // TODO achieve Render On Quad
    cmdList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    cmdList->IASetVertexBuffers(0, 1, &mQuadVertexView);
    cmdList->DrawInstanced(4, 1, 0, 0);
    //
}

void Scene::UpdateSceneConstant()
{
    using DirectX::XMMATRIX;
    mCamera.at("default").UpdateViewMatrix();
    SceneInfo sceneInfo = {};
    XMMATRIX view = mCamera["default"].GetView();
    XMMATRIX project = mCamera["default"].GetProj();
    XMMATRIX viewProject = DirectX::XMMatrixMultiply(view, project);

    auto viewDet = XMMatrixDeterminant(view);
    auto projectDet = XMMatrixDeterminant(project);
    auto viewProjectDet = DirectX::XMMatrixDeterminant(viewProject);

    auto invView = XMMatrixInverse(&viewDet, view);
    auto invProject = XMMatrixInverse(&projectDet, project);
    auto invViewProject = XMMatrixInverse(&viewProjectDet, viewProject);

    XMStoreFloat4x4(&sceneInfo.View, XMMatrixTranspose(view));
    XMStoreFloat4x4(&sceneInfo.InvView, XMMatrixTranspose(invView));
    XMStoreFloat4x4(&sceneInfo.Project, XMMatrixTranspose(invView));
    XMStoreFloat4x4(&sceneInfo.InvProject, XMMatrixTranspose(invView));
    XMStoreFloat4x4(&sceneInfo.ViewProject, XMMatrixTranspose(viewProject));
    XMStoreFloat4x4(&sceneInfo.InvViewProject, XMMatrixTranspose(invViewProject));

    sceneInfo.EyePosition = mCamera["default"].GetPosition3f();

    mSceneConstant->copyData(0, sceneInfo);
}

void Scene::UpdateEntityConstant()
{
    using namespace DirectX;
    for (auto &entity : mEntities) {
        EntityInfo info;
        auto transform = DirectX::XMLoadFloat4x4(&entity.Transform);
        XMStoreFloat4x4(&info.Transform, DirectX::XMMatrixTranspose(transform));
        info.MaterialIndex = entity.MaterialIndex;
        mObjectConstant->copyData(entity.EntityIndex, info);
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

void Scene::InitUI(ID3D12Device *device)
{

    ImGui::StyleColorsDark();
    ImGui_ImplWin32_Init(Application::GetHandle());

    // TODO 128 Relevate with UI count
    mUiSrvHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);

    ImGui_ImplDX12_Init(
        device,
        3,
        DXGI_FORMAT_R8G8B8A8_UNORM,
        mUiSrvHeap->Resource(),
        mUiSrvHeap->CPUHandle(0),
        mUiSrvHeap->GPUHandle(0));
    ImGui_ImplDX12_CreateDeviceObjects();
}