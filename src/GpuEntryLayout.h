#pragma once
#include <stdafx.h>
class GpuEntryLayout {
public:
    static ComPtr<ID3D12PipelineState> CreateGBufferPassPSO(ID3D12Device *device, ID3D12RootSignature *signature,
                                                            std::vector<DXGI_FORMAT> rtvFormats,
                                                            DXGI_FORMAT dsvFormat);
    static ComPtr<ID3D12PipelineState> CreateLightPassPSO(ID3D12Device *device, ID3D12RootSignature *signature);
    static ComPtr<ID3D12PipelineState> CreateSkyBoxPassPSO(ID3D12Device *device, ID3D12RootSignature *signature);
    static ComPtr<ID3D12PipelineState> CreateShadowPassPSO(ID3D12Device *device, ID3D12RootSignature *signature);
    static ComPtr<ID3D12PipelineState> CreateQuadPassPSO(ID3D12Device *device, ID3D12RootSignature *signature);

    static ComPtr<ID3D12PipelineState> CreateSobelPSO(ID3D12Device *device, ID3D12RootSignature *signature);

    static ComPtr<ID3D12RootSignature> CreateRenderSignature(ID3D12Device *device, uint gbufferNum);
    static ComPtr<ID3D12RootSignature> CreateComputeSignature(ID3D12Device *device);
    static std::vector<D3D12_INPUT_ELEMENT_DESC> CreateInputLayout();
    static std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> CreateStaticSamplers();
};