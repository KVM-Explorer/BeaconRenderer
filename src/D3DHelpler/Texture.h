#pragma once
#include "DDSTextureLoader12.h"
#include "Tools/FrameworkHelper.h"
#include <stdafx.h>

class Texture {
public:
    Texture(Texture &&) = default;
    Texture &operator=(Texture &&) = default;
    Texture &operator=(const Texture &) = delete;
    Texture(const Texture &) = delete;
    ~Texture()
    {
        mTexture.Reset();
        mUploader.Reset();
    }

    Texture(ID3D12Resource *resource, ID3D12Resource *uploader = nullptr); 
    Texture(ID3D12Device *device, ID3D12GraphicsCommandList *commandList,
            std::wstring path, bool isCube = false);
    Texture(ID3D12Device *device,
            DXGI_FORMAT format,
            uint width, uint height,
            D3D12_RESOURCE_FLAGS flags,
            bool isDepthTexture = false,
            D3D12_HEAP_FLAGS heapFlags = D3D12_HEAP_FLAG_NONE,
            D3D12_RESOURCE_STATES initState = D3D12_RESOURCE_STATE_GENERIC_READ);

    [[nodiscard]] ID3D12Resource *Resource() const
    {
        return mTexture.Get();
    }
    [[nodiscard]] D3D12_RESOURCE_DESC GetDesc()
    {
        return mTexture->GetDesc();
    }

private:
    Microsoft::WRL::ComPtr<ID3D12Resource> mTexture;
    Microsoft::WRL::ComPtr<ID3D12Resource> mUploader;
};
