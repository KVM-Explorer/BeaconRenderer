#pragma once
#include <stdafx.h>

class SobelFilter {
public:
    SobelFilter() = default;
    SobelFilter(const SobelFilter &) = default;
    SobelFilter(SobelFilter &&) = delete;
    SobelFilter &operator=(const SobelFilter &) = default;
    SobelFilter &operator=(SobelFilter &&) = delete;

    void Init(ID3D12Device *device);
    void Draw(ID3D12GraphicsCommandList *cmdList, D3D12_GPU_DESCRIPTOR_HANDLE srvHandle);
    ID3D12Resource *OuptputResource();
    [[nodiscard]] uint OutputSrvIndex() const
    {
        return mTextureSrvIndex;
    };

private:
    void CreateResource(ID3D12Device *device);
    void CreatePSO(ID3D12Device *device);
    void CreateRS(ID3D12Device *device);

    ComPtr<ID3D12RootSignature> mRS;
    ComPtr<ID3D12PipelineState> mPSO;
    uint mTextureSrvIndex;
    uint mTextureUvaIndex;
    uint mTextureResourceIndex;
};