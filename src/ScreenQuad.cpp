#include "ScreenQuad.h"
#include "Framework/GlobalResource.h"

void ScreenQuad::Init(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList)
{
    CreateVertices(device, cmdList);
    CompileShaders();
    CreateRootSignature(device);
    CreatePSO(device);
}


void ScreenQuad::CreateVertices(ID3D12Device *device, ID3D12GraphicsCommandList *cmdList)
{
    const std::array<ModelVertex, 4> vertices{{
        {{-1.0F, 1.0F, 0.0F},{} ,{0.0F, 0.0F}},
        {{1.0F, 1.0F, 0.0F}, {},{1.0F, 0.0F}},
        {{-1.0F, -1.0F, 0.0F},{} ,{0.0F, 1.0F}},
        {{1.0F, -1.0F, 0.0F}, {},{1.0F, 1.0F}},
    }};

    const uint verticesByteSize = vertices.size() * sizeof(ModelVertex);
    mQuadVertexBuffer = std::make_unique<DefaultBuffer>(device, cmdList, vertices.data(), verticesByteSize);

    mQuadVertexView.BufferLocation = mQuadVertexBuffer->Resource()->GetGPUVirtualAddress();
    mQuadVertexView.SizeInBytes = verticesByteSize;
    mQuadVertexView.StrideInBytes = sizeof(ModelVertex);
}

void ScreenQuad::CompileShaders()
{
    UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
    ComPtr<ID3DBlob> error;

    ThrowIfFailed(D3DCompileFromFile(L"Shaders/ScreenQuad.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "VSMain", "vs_5_1", compileFlags, 0, &mShaders["ScreenQuadVS"], &error));
    if (error != nullptr) {
        OutputDebugStringA(static_cast<char *>(error->GetBufferPointer()));
    }
    ThrowIfFailed(D3DCompileFromFile(L"Shaders/ScreenQuad.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "PSMain", "ps_5_1", compileFlags, 0, &mShaders["ScreenQuadPS"], &error));
    if (error != nullptr) {
        OutputDebugStringA(static_cast<char *>(error->GetBufferPointer()));
    }
}

void ScreenQuad::CreateRootSignature(ID3D12Device *device)
{
    CD3DX12_DESCRIPTOR_RANGE srv1Table;
    CD3DX12_DESCRIPTOR_RANGE srv2Table;
    srv1Table.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    srv2Table.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 1);

    std::array<CD3DX12_ROOT_PARAMETER, 3> rootParams;
    rootParams.at(0).InitAsConstants(1, 0);
    rootParams.at(1).InitAsDescriptorTable(1, &srv1Table);
    rootParams.at(2).InitAsDescriptorTable(1, &srv2Table);

    std::array<CD3DX12_STATIC_SAMPLER_DESC, 1> samplers = {{{
        0,                               // shaderRegister
        D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressU
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressV
        D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressW
    }}};

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(rootParams.size(),
                                            rootParams.data(),
                                            samplers.size(),
                                            samplers.data(),
                                            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> result;
    ComPtr<ID3DBlob> error;

    ThrowIfFailed(D3D12SerializeRootSignature(&rootSigDesc,
                                              D3D_ROOT_SIGNATURE_VERSION_1_0,
                                              &result, &error));
    if (error != nullptr) {
        OutputDebugStringA(static_cast<char *>(error->GetBufferPointer()));
    }
    ThrowIfFailed(device->CreateRootSignature(0,
                                              result->GetBufferPointer(),
                                              result->GetBufferSize(),
                                              IID_PPV_ARGS(&mRootSignature)));
}

void ScreenQuad::CreatePSO(ID3D12Device *device)
{
    D3D12_GRAPHICS_PIPELINE_STATE_DESC quadDesc = {};
    quadDesc.InputLayout = {GResource::InputLayout.data(), static_cast<uint>(GResource::InputLayout.size())};
    quadDesc.VS = CD3DX12_SHADER_BYTECODE(mShaders["ScreenQuadVS"].Get());
    quadDesc.PS = CD3DX12_SHADER_BYTECODE(mShaders["ScreenQuadPS"].Get());
    quadDesc.pRootSignature = mRootSignature.Get();
    quadDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    quadDesc.DepthStencilState.DepthEnable = false;
    quadDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    quadDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    quadDesc.RasterizerState.DepthClipEnable = false;
    quadDesc.SampleMask = UINT_MAX;
    quadDesc.NumRenderTargets = 1;
    quadDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    quadDesc.SampleDesc.Count = 1;
    quadDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    ThrowIfFailed(device->CreateGraphicsPipelineState(&quadDesc, IID_PPV_ARGS(&mPSO)));
}

void ScreenQuad::SetState(ID3D12GraphicsCommandList *cmdList, QuadShader shaderID)
{
    cmdList->SetGraphicsRootSignature(mRootSignature.Get());
    cmdList->SetPipelineState(mPSO.Get());
    uint data = static_cast<uint>(shaderID);
    cmdList->SetGraphicsRoot32BitConstants(0, 1, &data, 0);
}

void ScreenQuad::Draw(ID3D12GraphicsCommandList *cmdList)
{
    cmdList->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    cmdList->IASetVertexBuffers(0, 1, &mQuadVertexView);
    cmdList->DrawInstanced(4, 1, 0, 0);
}