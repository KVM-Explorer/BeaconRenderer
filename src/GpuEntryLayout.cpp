#include "GpuEntryLayout.h"
#include "Tools/FrameworkHelper.h"
#include "Framework/GlobalResource.h"

std::vector<D3D12_INPUT_ELEMENT_DESC> GpuEntryLayout::CreateInputLayout()
{
    return {{
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
        {"TEXCOORD",
         0,
         DXGI_FORMAT_R32G32_FLOAT,
         0,
         24,
         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
         0},
    }};
}
ComPtr<ID3D12RootSignature> GpuEntryLayout::CreateRenderSignature(ID3D12Device *device, uint gbufferNum)
{
    ComPtr<ID3D12RootSignature> ret;
    std::array<CD3DX12_DESCRIPTOR_RANGE, 1> srvRange = {};
    srvRange.at(0).Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, gbufferNum + 1, 0); // Gbuffer 3 output + Depth
    std::array<CD3DX12_DESCRIPTOR_RANGE, 1> texRange = {};
    texRange.at(0).Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 100, 0, 2); // Bindless Diffuse Texture

    std::array<CD3DX12_ROOT_PARAMETER, 7> rootParameters = {};
    rootParameters.at(0).InitAsConstantBufferView(0);                             // Object Constant
    rootParameters.at(1).InitAsConstantBufferView(1);                             // Scene Constant
    rootParameters.at(2).InitAsShaderResourceView(0, 1);                          // PointLight
    rootParameters.at(3).InitAsDescriptorTable(srvRange.size(), srvRange.data()); // Gbuffer 3 output + Depth
    rootParameters.at(4).InitAsDescriptorTable(texRange.size(), texRange.data()); // UV Texture
    rootParameters.at(5).InitAsShaderResourceView(1, 1);                          // Materials
    rootParameters.at(6).InitAsConstants(1, 0, 1);
    auto staticSamplers = CreateStaticSamplers();

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.Init(rootParameters.size(),
                           rootParameters.data(),
                           staticSamplers.size(),
                           staticSamplers.data(),
                           D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> signature;
    ComPtr<ID3DBlob> error;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
    if (error != nullptr) {
        OutputDebugStringA(static_cast<char *>(error->GetBufferPointer()));
    }
    ThrowIfFailed(device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(),
                                              IID_PPV_ARGS(&ret)));
    return ret;
}
ComPtr<ID3D12RootSignature> GpuEntryLayout::CreateComputeSignature(ID3D12Device *device)
{
    ComPtr<ID3D12RootSignature> signature;

    CD3DX12_DESCRIPTOR_RANGE srvTable;
    srvTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    CD3DX12_DESCRIPTOR_RANGE uavTable;
    uavTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0);
    std::array<CD3DX12_ROOT_PARAMETER, 2> rootParams;
    rootParams.at(0).InitAsDescriptorTable(1, &srvTable);
    rootParams.at(1).InitAsDescriptorTable(1, &uavTable);

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(rootParams.size(),
                                            rootParams.data(),
                                            0,

                                            nullptr,
                                            D3D12_ROOT_SIGNATURE_FLAG_NONE);

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
                                              IID_PPV_ARGS(&signature)));
    return signature;
}

ComPtr<ID3D12PipelineState> GpuEntryLayout::CreateGBufferPassPSO(ID3D12Device *device, ID3D12RootSignature *signature,
                                                                 std::vector<DXGI_FORMAT> rtvFormats,
                                                                 DXGI_FORMAT dsvFormat)
{
    ComPtr<ID3D12PipelineState> ret;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC gBufferDesc = {};
    gBufferDesc.VS = CD3DX12_SHADER_BYTECODE(GResource::Shaders["GBufferVS"].Get());
    gBufferDesc.PS = CD3DX12_SHADER_BYTECODE(GResource::Shaders["GBufferPS"].Get());
    gBufferDesc.InputLayout = {GResource::InputLayout.data(), static_cast<uint>(GResource::InputLayout.size())};
    gBufferDesc.pRootSignature = signature;
    gBufferDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    gBufferDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    gBufferDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    gBufferDesc.SampleMask = UINT_MAX;
    gBufferDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    gBufferDesc.NumRenderTargets = rtvFormats.size();
    for (uint i = 0; i < rtvFormats.size(); i++)
        gBufferDesc.RTVFormats[i] = rtvFormats[i];
    gBufferDesc.DSVFormat = dsvFormat;
    gBufferDesc.SampleDesc.Count = 1;
    ThrowIfFailed(device->CreateGraphicsPipelineState(&gBufferDesc, IID_PPV_ARGS(&ret)));
    return ret;
}

ComPtr<ID3D12PipelineState> GpuEntryLayout::CreateLightPassPSO(ID3D12Device *device, ID3D12RootSignature *signature)
{
    ComPtr<ID3D12PipelineState> ret;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC lightPassDesc = {};
    lightPassDesc.InputLayout = {GResource::InputLayout.data(), static_cast<uint>(GResource::InputLayout.size())};
    lightPassDesc.VS = CD3DX12_SHADER_BYTECODE(GResource::Shaders["LightPassVS"].Get());
    lightPassDesc.PS = CD3DX12_SHADER_BYTECODE(GResource::Shaders["LightPassPS"].Get());
    lightPassDesc.pRootSignature = signature;
    lightPassDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    lightPassDesc.DepthStencilState.DepthEnable = false;
    lightPassDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    lightPassDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    lightPassDesc.RasterizerState.DepthClipEnable = false;
    lightPassDesc.SampleMask = UINT_MAX;
    lightPassDesc.NumRenderTargets = 1;
    lightPassDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    lightPassDesc.SampleDesc.Count = 1;
    lightPassDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    ThrowIfFailed(device->CreateGraphicsPipelineState(&lightPassDesc, IID_PPV_ARGS(&ret)));
    return ret;
}

ComPtr<ID3D12PipelineState> GpuEntryLayout::CreateSobelPSO(ID3D12Device *device, ID3D12RootSignature *signature)
{
    ComPtr<ID3D12PipelineState> ret;
    D3D12_COMPUTE_PIPELINE_STATE_DESC computeDesc = {};
    computeDesc.pRootSignature = signature;
    computeDesc.CS = CD3DX12_SHADER_BYTECODE(GResource::Shaders["SobelCS"].Get());
    computeDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
    ThrowIfFailed(device->CreateComputePipelineState(&computeDesc, IID_PPV_ARGS(&ret)));
    return ret;
}

ComPtr<ID3D12PipelineState> GpuEntryLayout::CreateQuadPassPSO(ID3D12Device *device, ID3D12RootSignature *signature)
{
    ComPtr<ID3D12PipelineState> ret;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC quadDesc = {};
    quadDesc.InputLayout = {GResource::InputLayout.data(), static_cast<uint>(GResource::InputLayout.size())};
    quadDesc.VS = CD3DX12_SHADER_BYTECODE(GResource::Shaders["ScreenQuadVS"].Get());
    quadDesc.PS = CD3DX12_SHADER_BYTECODE(GResource::Shaders["ScreenQuadPS"].Get());
    quadDesc.pRootSignature = signature;
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
    ThrowIfFailed(device->CreateGraphicsPipelineState(&quadDesc, IID_PPV_ARGS(&ret)));
    return ret;
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GpuEntryLayout::CreateStaticSamplers()
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
    return {
        pointWrap, pointClamp,
        linearWrap, linearClamp,
        anisotropicWrap, anisotropicClamp};
}