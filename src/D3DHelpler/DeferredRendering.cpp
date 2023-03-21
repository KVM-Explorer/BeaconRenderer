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
    mRTVDescriptorHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 3);

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_RESOURCE_DESC rtDesc = CD3DX12_RESOURCE_DESC::Tex2D(mRTVFormat[0],
                                                              mScreenWidth,
                                                              mScreenHeight,
                                                              1,
                                                              1,
                                                              1,
                                                              0,
                                                              D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET);

    for (uint i = 0; i < mRTNum; i++) {
        rtDesc.Format = mRTVFormat.at(i);
        D3D12_CLEAR_VALUE clearValue{mRTVFormat.at(i)};
        ThrowIfFailed(device->CreateCommittedResource(&heapProps,
                                                      D3D12_HEAP_FLAG_NONE,
                                                      &rtDesc,
                                                      D3D12_RESOURCE_STATE_GENERIC_READ,
                                                      &clearValue,
                                                      IID_PPV_ARGS(&mGBuffer.at(i))));
    }

    D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
    rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;
    rtvDesc.Texture2D.PlaneSlice = 0;

    for (uint i = 0; i < mRTNum; i++) {
        rtvDesc.Format = mRTVFormat.at(i);
        device->CreateRenderTargetView(mGBuffer.at(i).Get(),
                                       &rtvDesc,
                                       mRTVDescriptorHeap->CPUHandle(i));
    }

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = rtDesc.MipLevels;
    srvDesc.Texture2D.MostDetailedMip = 0;

    mSRVDescriptorHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 4, true);
    for (uint i = 0; i < mRTNum; i++) {
        srvDesc.Format = mRTVFormat.at(i);
        device->CreateShaderResourceView(mGBuffer.at(i).Get(), &srvDesc, mSRVDescriptorHeap->CPUHandle(i));
    }
}

void DeferredRendering::CreateDSV(ID3D12Device *device)
{
    mDSVDescriptorHeap = std::make_unique<DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1);

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    D3D12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(mDSVFormat,
                                                                    mScreenWidth,
                                                                    mScreenHeight,
                                                                    1,
                                                                    1,
                                                                    1,
                                                                    0,
                                                                    D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
    D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
    depthOptimizedClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    depthOptimizedClearValue.DepthStencil.Depth = 1.0F;
    depthOptimizedClearValue.DepthStencil.Stencil = 0.0F;

    ThrowIfFailed(device->CreateCommittedResource(&heapProps,
                                                  D3D12_HEAP_FLAG_NONE,
                                                  &resourceDesc,
                                                  D3D12_RESOURCE_STATE_GENERIC_READ,
                                                  &depthOptimizedClearValue,
                                                  IID_PPV_ARGS(&mDepthTexture)));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDescriptor = {};
    dsvDescriptor.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDescriptor.Flags = D3D12_DSV_FLAG_NONE;
    dsvDescriptor.Format = mDSVFormat;
    dsvDescriptor.Texture2D.MipSlice = 0;
    device->CreateDepthStencilView(mDepthTexture.Get(), &dsvDescriptor, mDSVDescriptorHeap->CPUHandle(0));

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDescriptor = {};
    srvDescriptor.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDescriptor.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDescriptor.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS; // TODO other format ?
    srvDescriptor.Texture2D.MipLevels = resourceDesc.MipLevels;
    srvDescriptor.Texture2D.MostDetailedMip = 0;
    device->CreateShaderResourceView(mDepthTexture.Get(), &srvDescriptor, mSRVDescriptorHeap->CPUHandle(3));
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
    std::array<ID3D12DescriptorHeap *, 1> srvHeaps{mSRVDescriptorHeap->Resource()};

    cmdList->SetPipelineState(mGBufferPSO.Get());
    // TODO 重新设计Clear Value使其匹配其对应格式
    /*D3D12 WARNING: ID3D12CommandList::ClearRenderTargetView: The clear values do not match those passed to resource creation. The clear operation is typically slower as a result; but will still clear to the desired value. [ EXECUTION WARNING #820: CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE]*/

    for (uint i = 0; i < mRTNum; i++) {
        auto srv2rtv = CD3DX12_RESOURCE_BARRIER::Transition(mGBuffer.at(i).Get(),
                                                            D3D12_RESOURCE_STATE_GENERIC_READ,
                                                            D3D12_RESOURCE_STATE_RENDER_TARGET);
        cmdList->ResourceBarrier(1, &srv2rtv);
        cmdList->ClearRenderTargetView(mRTVDescriptorHeap->CPUHandle(i), DirectX::Colors::SteelBlue, 0, nullptr);
    }
    auto srv2depthBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mDepthTexture.Get(),
                                                                 D3D12_RESOURCE_STATE_GENERIC_READ,
                                                                 D3D12_RESOURCE_STATE_DEPTH_WRITE);
    cmdList->ResourceBarrier(1, &srv2depthBarrier);
    cmdList->ClearDepthStencilView(mDSVDescriptorHeap->CPUHandle(0), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0F, 0, 0, nullptr);
    auto rtvHandle = mRTVDescriptorHeap->CPUHandle(0);
    auto dsvHandle = mDSVDescriptorHeap->CPUHandle(0);
    cmdList->OMSetRenderTargets(mRTNum, &rtvHandle, true, &dsvHandle);
    cmdList->SetDescriptorHeaps(srvHeaps.size(), srvHeaps.data());
    cmdList->SetGraphicsRootSignature(mRootSignature.Get());
    cmdList->SetGraphicsRootDescriptorTable(2, mSRVDescriptorHeap->GPUHandle(0));
}

void DeferredRendering::LightingPass(ID3D12GraphicsCommandList *cmdList)
{
    for (uint i = 0; i < mRTNum; i++) {
        auto rtv2srvBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mGBuffer.at(i).Get(),
                                                                   D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                                   D3D12_RESOURCE_STATE_GENERIC_READ);
        cmdList->ResourceBarrier(1, &rtv2srvBarrier);
    }
    auto depth2srvBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mDepthTexture.Get(),
                                                                 D3D12_RESOURCE_STATE_DEPTH_WRITE,
                                                                 D3D12_RESOURCE_STATE_GENERIC_READ);
    cmdList->ResourceBarrier(1, &depth2srvBarrier);
    cmdList->SetPipelineState(mLightingPSO.Get());

    // TODO  Render Quad
}