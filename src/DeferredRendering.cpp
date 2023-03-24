#include "DeferredRendering.h"
#include "Tools/FrameworkHelper.h"

DeferredRendering::DeferredRendering(uint width, uint height) :
    mScreenHeight(height),
    mScreenWidth(width)
{
}

void DeferredRendering::Init(ID3D12Device *device)
{
    CreateRTV(device);
    CreateDSV(device);
    CreateRootSignature(device);
}

void DeferredRendering::CreateRTV(ID3D12Device *device)
{
    mRTVDescriptorHeap = std::make_unique<DescriptorHeap>(device,
                                                          D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
                                                          3);
    std::vector<Texture> targets;
    for (uint i = 0; i < mRTNum; i++) {
        Texture texture(device,
                        mRTVFormat[i],
                        mScreenWidth, mScreenHeight,
                        D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);
        targets.push_back(std::move(texture));
    }

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.Texture2D.PlaneSlice = 0;

    for (uint i = 0; i < mRTNum; i++) {
        rtvDesc.Format = mRTVFormat.at(i);
        device->CreateRenderTargetView(targets[i].Resource(),
                                       &rtvDesc,
                                       mRTVDescriptorHeap->CPUHandle(i));
    }

    for (uint i = 0; i < mRTNum; i++) {
        uint index = GResource::TextureManager->StoreTexture(targets[i], targets[i].GetDesc().Format);
        mGbufferTextureIndex.at(i) = static_cast<int>(index);
    }
}
void DeferredRendering::CreateDSV(ID3D12Device *device)
{
    mDSVDescriptorHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);

    Texture texture(device, mDSVFormat,
                    mScreenWidth, mScreenHeight,
                    D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
                    true);

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDescriptor = {};
    dsvDescriptor.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDescriptor.Flags = D3D12_DSV_FLAG_NONE;
    dsvDescriptor.Format = mDSVFormat;
    dsvDescriptor.Texture2D.MipSlice = 0;
    device->CreateDepthStencilView(texture.Resource(), &dsvDescriptor, mDSVDescriptorHeap->CPUHandle(0));

    uint index = GResource::TextureManager->StoreTexture(texture, DXGI_FORMAT_R24_UNORM_X8_TYPELESS);
    mDepthTextureIndex = static_cast<int>(index);
}

void DeferredRendering::CreateRootSignature(ID3D12Device *device)
{
    std::array<CD3DX12_DESCRIPTOR_RANGE, 1> srvRange;
    srvRange.at(0).Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0);

    std::array<CD3DX12_ROOT_PARAMETER, 3> rootParameters;
    rootParameters.at(0).InitAsConstantBufferView(0); // Object Constant
    rootParameters.at(1).InitAsConstantBufferView(1); // Scene Constant
    rootParameters.at(2).InitAsDescriptorTable(srvRange.size(), srvRange.data());

    std::array<CD3DX12_STATIC_SAMPLER_DESC, 1> staticSamplers;
    staticSamplers[0].Init(0, D3D12_FILTER_MIN_MAG_MIP_LINEAR);

    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.Init(rootParameters.size(),
                           rootParameters.data(),
                           staticSamplers.size(),
                           staticSamplers.data(),
                           D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ComPtr<ID3DBlob> rootBlob, errorBlob;
    ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &rootBlob, &errorBlob));
    ThrowIfFailed(device->CreateRootSignature(0, rootBlob->GetBufferPointer(), rootBlob->GetBufferSize(),
                                              IID_PPV_ARGS(&mRootSignature)));
}

void DeferredRendering::CreatePSOs(ID3D12Device *device,
                                   std::vector<D3D12_INPUT_ELEMENT_DESC> gBufferInputLayout,
                                   ComPtr<ID3DBlob> gBufferVS,
                                   ComPtr<ID3DBlob> gBufferPS,
                                   std::vector<D3D12_INPUT_ELEMENT_DESC> lightPassInputLayout,
                                   ComPtr<ID3DBlob> QuadRenderingVS,
                                   ComPtr<ID3DBlob> QuadRenderingPS)
{
    // CreatePSO
    D3D12_GRAPHICS_PIPELINE_STATE_DESC gBufferDesc = {};
    gBufferDesc.VS = CD3DX12_SHADER_BYTECODE(gBufferVS.Get());
    gBufferDesc.PS = CD3DX12_SHADER_BYTECODE(gBufferPS.Get());
    gBufferDesc.InputLayout = {gBufferInputLayout.data(), static_cast<uint>(gBufferInputLayout.size())};
    gBufferDesc.pRootSignature = mRootSignature.Get();
    gBufferDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    gBufferDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    gBufferDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    gBufferDesc.SampleMask = UINT_MAX;
    gBufferDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    gBufferDesc.NumRenderTargets = 3;
    gBufferDesc.NumRenderTargets = mRTNum;
    gBufferDesc.RTVFormats[0] = mRTVFormat[0];
    gBufferDesc.RTVFormats[1] = mRTVFormat[1];
    gBufferDesc.RTVFormats[2] = mRTVFormat[2];
    gBufferDesc.DSVFormat = mDSVFormat;
    gBufferDesc.SampleDesc.Count = 1;
    ThrowIfFailed(device->CreateGraphicsPipelineState(&gBufferDesc, IID_PPV_ARGS(&mGBufferPSO)));

    D3D12_GRAPHICS_PIPELINE_STATE_DESC lightPassDesc = {};
    lightPassDesc.InputLayout = {lightPassInputLayout.data(), static_cast<uint>(lightPassInputLayout.size())};
    lightPassDesc.VS = CD3DX12_SHADER_BYTECODE(QuadRenderingVS.Get());
    lightPassDesc.PS = CD3DX12_SHADER_BYTECODE(QuadRenderingPS.Get());
    lightPassDesc.pRootSignature = mRootSignature.Get();
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
    ThrowIfFailed(device->CreateGraphicsPipelineState(&lightPassDesc, IID_PPV_ARGS(&mLightingPSO)));
}

void DeferredRendering::GBufferPass(ID3D12GraphicsCommandList *cmdList)
{
    std::array<ID3D12DescriptorHeap *, 1> srvHeaps{GResource::TextureManager->GetTexture2DDescriptoHeap()->Resource()};

    cmdList->SetPipelineState(mGBufferPSO.Get());

    float clearColor[4] = {0, 0.0F, 0.0F, 1.0F};
    for (uint i = 0; i < mRTNum; i++) {
        auto rtvTexture = GResource::TextureManager->GetTexture(mGbufferTextureIndex.at(i));
        auto srv2rtv = CD3DX12_RESOURCE_BARRIER::Transition(rtvTexture->Resource(),
                                                            D3D12_RESOURCE_STATE_GENERIC_READ,
                                                            D3D12_RESOURCE_STATE_RENDER_TARGET);
        cmdList->ResourceBarrier(1, &srv2rtv);
        cmdList->ClearRenderTargetView(mRTVDescriptorHeap->CPUHandle(i), clearColor, 0, nullptr);
    }
    auto depthTexture = GResource::TextureManager->GetTexture(mDepthTextureIndex);
    auto srv2depthBarrier = CD3DX12_RESOURCE_BARRIER::Transition(depthTexture->Resource(),
                                                                 D3D12_RESOURCE_STATE_GENERIC_READ,
                                                                 D3D12_RESOURCE_STATE_DEPTH_WRITE);
    cmdList->ResourceBarrier(1, &srv2depthBarrier);
    cmdList->ClearDepthStencilView(mDSVDescriptorHeap->CPUHandle(0), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0F, 0, 0, nullptr);
    auto rtvHandle = mRTVDescriptorHeap->CPUHandle(0);
    auto dsvHandle = mDSVDescriptorHeap->CPUHandle(0);
    cmdList->OMSetRenderTargets(mRTNum, &rtvHandle, true, &dsvHandle);
    cmdList->SetDescriptorHeaps(srvHeaps.size(), srvHeaps.data());
    cmdList->SetGraphicsRootSignature(mRootSignature.Get());
    cmdList->SetGraphicsRootDescriptorTable(2, GResource::TextureManager->GetTexture2DDescriptoHeap()->GPUHandle(0));
}

void DeferredRendering::LightingPass(ID3D12GraphicsCommandList *cmdList)
{
    for (uint i = 0; i < mRTNum; i++) {
        auto rtvTexture = GResource::TextureManager->GetTexture(mGbufferTextureIndex.at(i));
        auto rtv2srvBarrier = CD3DX12_RESOURCE_BARRIER::Transition(rtvTexture->Resource(),
                                                                   D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                                   D3D12_RESOURCE_STATE_GENERIC_READ);
        cmdList->ResourceBarrier(1, &rtv2srvBarrier);
    }
    auto depthTexture = GResource::TextureManager->GetTexture(mDepthTextureIndex);
    auto depth2srvBarrier = CD3DX12_RESOURCE_BARRIER::Transition(depthTexture->Resource(),
                                                                 D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                                                 D3D12_RESOURCE_STATE_GENERIC_READ);
    cmdList->ResourceBarrier(1, &depth2srvBarrier);
    cmdList->SetPipelineState(mLightingPSO.Get());

    // TODO  Render Quad
}